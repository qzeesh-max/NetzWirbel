/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdint>
#include <limits>
#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
using socklen_t = int;
#define close closesocket
#ifdef _MSC_VER
using ssize_t = int;
#endif
#undef max
#undef min
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include "../OdysseyTrader/Protocol.h"
#include "../OdysseyTrader/OverAlignedAllocator.hpp"
#include "OrderMatcher.h"
#include <random>

std::random_device g_rd;
std::mt19937_64 g_rng(g_rd()); // Mersenne Twister
std::uniform_int_distribution<uint64_t> g_dist(0, std::numeric_limits<uint64_t>::max()); 

// Non-blocking socket helper
bool set_nonblocking(int fd) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

struct ClientConnection {
    int fd;
    std::string username;
    std::set<std::string> subscriptions;
    std::vector<uint8_t, OverAlignedAllocator<uint8_t, 16>> read_buf;
};

using OrdersMap = std::unordered_map<std::string, Order>;
using OrdersMapByUser = std::unordered_map<std::string, OrdersMap>;
// Historical databases
OrdersMapByUser g_orders;

struct ExecutionRecord {
    std::string cl_ord_id;
    std::string symbol;
    uint8_t side;
    double price;
    uint32_t qty;
    double exec_px;
    uint32_t exec_qty;
    std::string owner;
};


using ExecutionsMapByUser = std::unordered_map<std::string, std::vector<ExecutionRecord>>;
ExecutionsMapByUser g_executions;

struct RejectRecord {
    std::string cl_ord_id;
    std::string orig_cl_ord_id;
    std::string symbol;
    uint8_t side;
    std::string reason;
    uint8_t is_cancel_replace_reject;
    std::string owner;
};

using RejectionsList = std::vector<RejectRecord>;
using RejectionsListByUser = std::unordered_map<std::string, RejectionsList>;
RejectionsListByUser g_rejections;

OrderMatcher g_matcher;
std::vector<std::shared_ptr<ClientConnection>> g_clients;

std::string generateOrderId() {
    unsigned long a = g_dist(g_rng) & 0xFFFF;
    unsigned long b = g_dist(g_rng) & 0xFFFF;
    unsigned long c = g_dist(g_rng) & 0xFFFF;
    char buf[64];
    snprintf(buf, sizeof(buf), "%04lx-%04lx-%04lx", a, b, c);
    return std::string(buf);
}

// Prepopulate database helper
void prepopulate() {
    // trader1 prepopulations
    Order o1(generateOrderId(), "LNUX", "trader1", Order::buy, 50.00, 100);
    Order o2(generateOrderId(), "YHOO", "trader1", Order::buy, 20.00, 500);
    Order o3(generateOrderId(), "CSCO", "trader1", Order::buy, 30.00, 1000);
    
    // trader2 prepopulations
    Order o4(generateOrderId(), "LNUX", "trader2", Order::sell, 50.50, 200);
    Order o5(generateOrderId(), "YHOO", "trader2", Order::sell, 20.10, 400);
    Order o6(generateOrderId(), "CSCO", "trader2", Order::sell, 30.20, 800);

    auto &to1 = g_orders["trader1"];
    to1.emplace(o1.getClOrdID(), o1);
    to1.emplace(o2.getClOrdID(), o2);
    to1.emplace(o3.getClOrdID(), o3);
    
    auto &to2 = g_orders["trader2"];
    to2.emplace(o4.getClOrdID(), o4);
    to2.emplace(o5.getClOrdID(), o5);
    to2.emplace(o6.getClOrdID(), o6);
   

    g_matcher.insert(o1);
    g_matcher.insert(o2);
    g_matcher.insert(o3);
    g_matcher.insert(o4);
    g_matcher.insert(o5);
    g_matcher.insert(o6);
}

void send_buffer(int fd, const void* data, size_t size) {
    size_t total_sent = 0;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
    while (total_sent < size) {
        ssize_t sent = send(fd, reinterpret_cast<const char*>(ptr + total_sent), size - total_sent, 0);
        if (sent <= 0) {
            if (sent < 0 && (
#ifdef _WIN32
                WSAGetLastError() == WSAEWOULDBLOCK
#else
                errno == EAGAIN || errno == EWOULDBLOCK
#endif
            )) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Wait briefly
                continue;
            }
            break;
        }
        total_sent += static_cast<size_t>(sent);
    }
}

void send_heartbeat(int fd) {
    MsgHeader hdr;
    hdr.type = htons(static_cast<uint16_t>(MsgType::Heartbeat));
    hdr.length = htonl(sizeof(MsgHeader));

    send_buffer(fd, &hdr, sizeof(hdr));
}

void strncpy_safe(char* dst, const char* src, size_t n) {
    std::strncpy(dst, src, n);
    dst[n - 1] = '\0';
}

void broadcast_market_data(const std::string& symbol) {
    const Market* mkt = g_matcher.getMarket(symbol);
    if (!mkt) return;

    NetMsg<MDUpdateMsg> msg;
    msg.header.type = htons(static_cast<uint16_t>(MsgType::MDUpdate));
    msg.header.length = htonl(sizeof(msg));
    
    strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
    
    double bid_px = 0.0, ask_px = 0.0;
    uint32_t bid_sz = 0, ask_sz = 0;
    if (mkt->getBestBid(bid_px, bid_sz)) {
        msg.payload.bid_px = bid_px;
        msg.payload.bid_size = bid_sz;
    }
    if (mkt->getBestAsk(ask_px, ask_sz)) {
        msg.payload.ask_px = ask_px;
        msg.payload.ask_size = ask_sz;
    }
    msg.payload.last_px = mkt->getLastTradePrice();
    msg.payload.last_size = mkt->getLastTradeSize();
    msg.payload.total_volume = mkt->getTotalVolume();

    for (const auto& client : g_clients) {
        if (!client->username.empty() && client->subscriptions.count(symbol)) {
            send_buffer(client->fd, &msg, sizeof(msg));
        }
    }
}

void handle_logon(const std::shared_ptr<ClientConnection>& client, const LogonReqMsg& req) {
    client->username = req.username;
    std::cout << "User '" << client->username << "' logged on successfully." << std::endl;

    // Send Logon Response
    NetMsg<LogonRespMsg> msg;
    msg.header.type = htons(static_cast<uint16_t>(MsgType::LogonResp));
    msg.header.length = htonl(sizeof(msg));
    msg.payload.status = 0; // Success
    strncpy_safe(msg.payload.session_name, "ODYSSEY_SESSION", sizeof(msg.payload.session_name));
    send_buffer(client->fd, &msg, sizeof(msg));

    // Construct and send LogonStateMsg
    std::vector<LogonStateOrder> user_orders;
    for (const auto& [_, o] : g_orders[client->username]) {
            LogonStateOrder lso;
            strncpy_safe(lso.cl_ord_id, o.getClOrdID().c_str(), sizeof(lso.cl_ord_id));
            strncpy_safe(lso.symbol, o.getSymbol().c_str(), sizeof(lso.symbol));
            lso.side = static_cast<uint8_t>(o.getSide());
            lso.price = o.getPrice();
            lso.qty = o.getQuantity();
            lso.cum_qty = o.getExecutedQuantity();
            lso.status = o.getStatus();
            user_orders.push_back(lso);
    }

    std::vector<LogonStateExecution> user_execs;
    for (const auto& e : g_executions[client->username]) {
            LogonStateExecution lse;
            strncpy_safe(lse.cl_ord_id, e.cl_ord_id.c_str(), sizeof(lse.cl_ord_id));
            strncpy_safe(lse.symbol, e.symbol.c_str(), sizeof(lse.symbol));
            lse.side = e.side;
            lse.price = e.price;
            lse.qty = e.qty;
            lse.exec_px = e.exec_px;
            lse.exec_qty = e.exec_qty;
            user_execs.push_back(lse);
    }

    std::vector<LogonStateReject> user_rejects;
    for (const auto& r : g_rejections[client->username]) {
        LogonStateReject lsr;
        strncpy_safe(lsr.cl_ord_id, r.cl_ord_id.c_str(), sizeof(lsr.cl_ord_id));
        strncpy_safe(lsr.orig_cl_ord_id, r.orig_cl_ord_id.c_str(), sizeof(lsr.orig_cl_ord_id));
        strncpy_safe(lsr.symbol, r.symbol.c_str(), sizeof(lsr.symbol));
        lsr.side = r.side;
        strncpy_safe(lsr.reason, r.reason.c_str(), sizeof(lsr.reason));
        lsr.is_cancel_replace_reject = r.is_cancel_replace_reject;
        user_rejects.push_back(lsr);
    }

    NetMsg<LogonStateMsg> stateMsg;
    stateMsg.header.type = htons(static_cast<uint16_t>(MsgType::LogonState));
    stateMsg.payload.order_count = user_orders.size();
    stateMsg.payload.execution_count = user_execs.size();
    stateMsg.payload.reject_count = user_rejects.size();

    size_t payload_len = sizeof(stateMsg) + 
                         user_orders.size() * sizeof(LogonStateOrder) +
                         user_execs.size() * sizeof(LogonStateExecution) +
                         user_rejects.size() * sizeof(LogonStateReject);

    stateMsg.header.length = htonl(payload_len);

    std::vector<uint8_t> payload(payload_len);
    size_t offset = 0;
    
    std::memcpy(payload.data() + offset, &stateMsg, sizeof(stateMsg));
    offset += sizeof(stateMsg);
    
    if (!user_orders.empty()) {
        std::memcpy(payload.data() + offset, user_orders.data(), user_orders.size() * sizeof(LogonStateOrder));
        offset += user_orders.size() * sizeof(LogonStateOrder);
    }
    if (!user_execs.empty()) {
        std::memcpy(payload.data() + offset, user_execs.data(), user_execs.size() * sizeof(LogonStateExecution));
        offset += user_execs.size() * sizeof(LogonStateExecution);
    }
    if (!user_rejects.empty()) {
        std::memcpy(payload.data() + offset, user_rejects.data(), user_rejects.size() * sizeof(LogonStateReject));
        offset += user_rejects.size() * sizeof(LogonStateReject);
    }

    send_buffer(client->fd, payload.data(), payload.size());
}

void process_matching_results(std::queue<Order>& updates, const std::string& symbol) {
    while (!updates.empty()) {
        Order o = updates.front();
        updates.pop();

        // Update global orders db
        auto &user_orders = g_orders[o.getOwner()];
        auto it = user_orders.find(o.getClOrdID());
        if (it != user_orders.end()) {
            it->second = o;
        }

        // Send execution reports to relevant client connections
        NetMsg<ExecReportMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::ExecReport));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.cl_ord_id, o.getClOrdID().c_str(), sizeof(msg.payload.cl_ord_id));
        std::stringstream ss;
        ss << "EXE" << rand() % 100000;
        strncpy_safe(msg.payload.order_id, ss.str().c_str(), sizeof(msg.payload.order_id));
        msg.payload.status = o.getStatus();
        strncpy_safe(msg.payload.symbol, o.getSymbol().c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = static_cast<uint8_t>(o.getSide());
        msg.payload.price = o.getPrice();
        msg.payload.qty = o.getQuantity();
        msg.payload.last_qty = o.getLastExecutedQuantity();
        msg.payload.last_px = o.getLastExecutedPrice();
        msg.payload.cum_qty = o.getExecutedQuantity();
        msg.payload.leaves_qty = o.getOpenQuantity();
        
        if (msg.payload.status == 2) {
            strncpy_safe(msg.payload.text, "Fully filled", sizeof(msg.payload.text));
        } else {
            strncpy_safe(msg.payload.text, "Partially filled", sizeof(msg.payload.text));
        }

        // Record execution report historically
        ExecutionRecord rec;
        rec.cl_ord_id = o.getClOrdID();
        rec.symbol = o.getSymbol();
        rec.side = static_cast<uint8_t>(o.getSide());
        rec.price = o.getPrice();
        rec.qty = o.getQuantity();
        rec.exec_px = o.getLastExecutedPrice();
        rec.exec_qty = o.getLastExecutedQuantity();
        rec.owner = o.getOwner();
        g_executions[o.getOwner()].push_back(rec);

        // Notify client(s)
        for (const auto& c : g_clients) {
            if (c->username == o.getOwner()) {
                send_buffer(c->fd, &msg, sizeof(msg));
            }
        }
    }

    broadcast_market_data(symbol);
}

void handle_new_order(const std::shared_ptr<ClientConnection>& client, const NewOrderMsg& req) {
    Order o(req.cl_ord_id, req.symbol, client->username, static_cast<Order::Side>(req.side), req.price, req.qty);
    g_orders[client->username].emplace(o.getClOrdID(), o);
    g_matcher.insert(o);

    std::cout << "NewOrder - ID: " << o.getClOrdID() << " Sym: " << o.getSymbol() 
              << " Qty: " << o.getQuantity() << " Price: " << o.getPrice() << " User: " << client->username << std::endl;

    // Send initial ACK (Pending/New Execution Report)
    NetMsg<ExecReportMsg> msg;
    msg.header.type = htons(static_cast<uint16_t>(MsgType::ExecReport));
    msg.header.length = htonl(sizeof(msg));
    strncpy_safe(msg.payload.cl_ord_id, o.getClOrdID().c_str(), sizeof(msg.payload.cl_ord_id));
    strncpy_safe(msg.payload.order_id, o.getClOrdID().c_str(), sizeof(msg.payload.order_id));
    msg.payload.status = 0; // '0' = New
    strncpy_safe(msg.payload.symbol, o.getSymbol().c_str(), sizeof(msg.payload.symbol));
    msg.payload.side = req.side;
    msg.payload.price = o.getPrice();
    msg.payload.qty = o.getQuantity();
    msg.payload.cum_qty = 0;
    msg.payload.leaves_qty = o.getOpenQuantity();
    strncpy_safe(msg.payload.text, "Order accepted", sizeof(msg.payload.text));
    send_buffer(client->fd, &msg, sizeof(msg));

    // Run matching
    std::queue<Order> updates;
    g_matcher.match(o.getSymbol(), updates);
    process_matching_results(updates, o.getSymbol());
}

void handle_replace_order(const std::shared_ptr<ClientConnection>& client, const OrderReplaceMsg& req) {
    std::string symbol = req.symbol;
    std::string orig_id = req.orig_cl_ord_id;
    std::string new_id = req.new_cl_ord_id;
    Order::Side side = static_cast<Order::Side>(req.side);

    try {
        Order target = g_matcher.find(symbol, side, orig_id);
        if (target.getOwner() != client->username) {
            throw std::runtime_error("Order owner mismatch");
        }

        g_matcher.erase(target);
        target.replace(new_id, req.price, req.qty);
        g_matcher.insert(target);


        // Update global database
        if (auto it = g_orders[client->username].find(orig_id); it != g_orders[client->username].end()) {
            it->second.replace(new_id, req.price, req.qty);
        }

        // Send ACK
        NetMsg<ExecReportMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::ExecReport));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.cl_ord_id, new_id.c_str(), sizeof(msg.payload.cl_ord_id));
        strncpy_safe(msg.payload.order_id, new_id.c_str(), sizeof(msg.payload.order_id));
        strncpy_safe(msg.payload.orig_cl_ord_id, orig_id.c_str(), sizeof(msg.payload.orig_cl_ord_id));
        msg.payload.status = 5; // '5' = Replaced / Replaced ACK
        strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = req.side;
        msg.payload.price = req.price;
        msg.payload.qty = req.qty;
        msg.payload.cum_qty = target.getExecutedQuantity();
        msg.payload.leaves_qty = target.getOpenQuantity();
        strncpy_safe(msg.payload.text, "Order replaced successfully", sizeof(msg.payload.text));
        send_buffer(client->fd, &msg, sizeof(msg));

        // Match
        std::queue<Order> updates;
        g_matcher.match(symbol, updates);
        process_matching_results(updates, symbol);

    } catch (std::exception& e) {
        NetMsg<CancelRejectMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::CancelReject));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.cl_ord_id, new_id.c_str(), sizeof(msg.payload.cl_ord_id));
        strncpy_safe(msg.payload.orig_cl_ord_id, orig_id.c_str(), sizeof(msg.payload.orig_cl_ord_id));
        strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = req.side;
        strncpy_safe(msg.payload.reason, e.what(), sizeof(msg.payload.reason));
        
        // Save reject record
        RejectRecord rec;
        rec.cl_ord_id = new_id;
        rec.orig_cl_ord_id = orig_id;
        rec.symbol = symbol;
        rec.side = req.side;
        rec.reason = e.what();
        rec.is_cancel_replace_reject = 1;
        rec.owner = client->username;
        g_rejections[client->username].push_back(rec);

        send_buffer(client->fd, &msg, sizeof(msg));
    }
}

void handle_cancel_order(const std::shared_ptr<ClientConnection>& client, const OrderCancelMsg& req) {
    std::string symbol = req.symbol;
    std::string orig_id = req.orig_cl_ord_id;
    Order::Side side = static_cast<Order::Side>(req.side);

    try {
        Order& target = g_matcher.find(symbol, side, orig_id);
        if (target.getOwner() != client->username) {
            throw std::runtime_error("Order owner mismatch");
        }

        target.cancel();
        g_matcher.erase(target);
 

        // Update global database
        auto &user_orders = g_orders[client->username];
        auto it = user_orders.find(orig_id);
        if (it != user_orders.end()) {
            it->second.cancel();
        }

        // Send canceled report
        NetMsg<ExecReportMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::ExecReport));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.cl_ord_id, orig_id.c_str(), sizeof(msg.payload.cl_ord_id));
        strncpy_safe(msg.payload.order_id, orig_id.c_str(), sizeof(msg.payload.order_id));
        msg.payload.status = 4; // '4' = Canceled
        strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = req.side;
        msg.payload.price = target.getPrice();
        msg.payload.qty = target.getQuantity();
        msg.payload.cum_qty = target.getExecutedQuantity();
        msg.payload.leaves_qty = 0;
        strncpy_safe(msg.payload.text, "Order canceled", sizeof(msg.payload.text));
        send_buffer(client->fd, &msg, sizeof(msg));

        // Re-broadcast BBO
        broadcast_market_data(symbol);

    } catch (std::exception& e) {
        NetMsg<CancelRejectMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::CancelReject));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.cl_ord_id, orig_id.c_str(), sizeof(msg.payload.cl_ord_id));
        strncpy_safe(msg.payload.orig_cl_ord_id, orig_id.c_str(), sizeof(msg.payload.orig_cl_ord_id));
        strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = req.side;
        strncpy_safe(msg.payload.reason, e.what(), sizeof(msg.payload.reason));

        // Save reject record
        RejectRecord rec;
        rec.cl_ord_id = orig_id;
        rec.orig_cl_ord_id = orig_id;
        rec.symbol = symbol;
        rec.side = req.side;
        rec.reason = e.what();
        rec.is_cancel_replace_reject = 0; // 0 = Order cancel reject
        rec.owner = client->username;
        g_rejections[client->username].push_back(rec);

        send_buffer(client->fd, &msg, sizeof(msg));
    }
}

bool handle_client_data(const std::shared_ptr<ClientConnection>& client) {
    const auto *data = client->read_buf.data();
    size_t remaining_size = client->read_buf.size();
    while (remaining_size >= sizeof(MsgHeader)) {
        auto &hdr = *reinterpret_cast<const MsgHeader*>(data);
        uint16_t type = ntohs(hdr.type);
        uint32_t length = ntohl(hdr.length);

        if (length & 15) [[unlikely]]
        {
            std::cerr << "Invalid message length: " << length << " - protocol requires length to be divisible by 16" << std::endl;
            return false;
        }

        if (remaining_size < length) return true;

        // Process message
        MsgType msg_type = static_cast<MsgType>(type);
        if (msg_type == MsgType::LogonReq) {
            const auto &req = reinterpret_cast<const NetMsg<LogonReqMsg>*>(data)->payload;
            handle_logon(client, req);
        } else if (client->username.empty()) {
            std::cout << "Received non-logon message on unauthenticated connection. Disconnecting." << std::endl;
            return false;
        } else {
            switch (msg_type) {
                case MsgType::NewOrder: {
                    const auto &req = reinterpret_cast<const NetMsg<NewOrderMsg>*>(data)->payload;
                    handle_new_order(client, req);
                    break;
                }
                case MsgType::OrderReplace: {
                    const auto&req = reinterpret_cast<const NetMsg<OrderReplaceMsg>*>(data)->payload;
                    handle_replace_order(client, req);
                    break;
                }
                case MsgType::OrderCancel: {
                    const auto &req = reinterpret_cast<const NetMsg<OrderCancelMsg>*>(data)->payload;
                    handle_cancel_order(client, req);
                    break;
                }
                case MsgType::MDRequest: {
                    const auto &req = reinterpret_cast<const NetMsg<MDRequestMsg>*>(data)->payload;
                    std::string sym = req.symbol;
                    if (req.sub_type == 0) {
                        client->subscriptions.insert(sym);
                        std::cout << "User '" << client->username << "' subscribed to " << sym << std::endl;
                        // Immediately broadcast snapshot to this client
                        broadcast_market_data(sym);
                    } else {
                        client->subscriptions.erase(sym);
                        std::cout << "User '" << client->username << "' unsubscribed from " << sym << std::endl;
                    }
                    break;
                }
                case MsgType::Heartbeat: {
                    std::cout << "OdysseyBackend received heartbeat from client " << client->username << ", echoing..." << std::endl;
                    send_heartbeat(client->fd);
                    break;
                }
                default:
                    std::cout << "Unhandled msg type: " << type << std::endl;
                    break;
            }
        }
        data += length;
        remaining_size -= length;
    }
    client->read_buf.erase(client->read_buf.begin(), client->read_buf.begin() + (client->read_buf.size() - remaining_size));

    return true;
}

int main(int argc, char** argv) {
    prepopulate();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket." << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(6001);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to port 6001" << std::endl;
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Failed to listen." << std::endl;
        return 1;
    }

    set_nonblocking(server_fd);
    std::cout << "Odyssey Trading Backend running on port 6001..." << std::endl;

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        
        int max_fd = server_fd;
        for (const auto& c : g_clients) {
            FD_SET(c->fd, &read_fds);
            if (c->fd > max_fd) max_fd = c->fd;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000; // 50ms check
        
        int ret = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret < 0) {
#ifdef _WIN32
            break;
#else
            if (errno == EINTR) continue;
            break;
#endif
        }

        // New connection
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                set_nonblocking(client_fd);
                auto conn = std::make_shared<ClientConnection>();
                conn->fd = client_fd;
                g_clients.push_back(conn);
                std::cout << "Client socket " << client_fd << " connected." << std::endl;
            }
        }

        // Existing client data read
        for (auto it = g_clients.begin(); it != g_clients.end();) {
            auto client = *it;
            if (FD_ISSET(client->fd, &read_fds)) {
                uint8_t buffer[4096];
                ssize_t read_bytes = recv(client->fd, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
                if (read_bytes <= 0) {
                    if (read_bytes < 0 && (
#ifdef _WIN32
                        WSAGetLastError() == WSAEWOULDBLOCK
#else
                        errno == EAGAIN || errno == EWOULDBLOCK
#endif
                    )) {
                        ++it;
                        continue;
                    }
                    std::cout << "Client socket " << client->fd << " disconnected." << std::endl;
                    close(client->fd);
                    it = g_clients.erase(it);
                    continue;
                } else {
                    client->read_buf.insert(client->read_buf.end(), buffer, buffer + read_bytes);
                    if (!handle_client_data(client))
                    {
                        close(client->fd);
                        it = g_clients.erase(it);
                        continue;
                    }
                }
            }
            ++it;
        }
    }

    close(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
