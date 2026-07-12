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

#include "Market.h"
#include <iostream>
#include <stdexcept>

namespace {
    template<bool OrderIteratorsSame, typename OrderMap>
    struct ChooseHelper;
    
    template <typename OrderMap>
    struct ChooseHelper<true, OrderMap>
    {
        static Order& find(OrderMap& orderMap_, Order::Side side, const std::string& id) {
            auto it = orderMap_.find(id);
            if (it == orderMap_.end()) {
                throw std::runtime_error("Order not found");
            }
            
            return it->second->second;
        }

        static void erase(OrderMap& orderMap_, Order::Side side, Market::BidOrders& bidOrders_, Market::AskOrders& askOrders_, const std::string& id) {
        auto it = orderMap_.find(id);
            if (it == orderMap_.end()) {
                throw std::runtime_error("Order not found");
            }
    
            if (side == Order::buy) {
                bidOrders_.erase(it->second);
            } else {
                askOrders_.erase(it->second);
            }

            orderMap_.erase(it);
        }   
    };

    template <typename OrderMap>
    struct ChooseHelper<false, OrderMap>
    {
        static Order& find(OrderMap& orderMap_, Order::Side side, const std::string& id) {
            auto it = orderMap_.find(id);
            if (it == orderMap_.end()) {
                throw std::runtime_error("Order not found");
            }
            
            if (side == Order::buy) {
                return std::get<Market::BidOrders::iterator>(it->second)->second;
            } else {
                return std::get<Market::AskOrders::iterator>(it->second)->second;
            }
        }

        static void erase(OrderMap& orderMap_, Order::Side side, Market::BidOrders& bidOrders_, Market::AskOrders& askOrders_, const std::string& id) {
             auto it = orderMap_.find(id);
            if (it == orderMap_.end()) {
                throw std::runtime_error("Order not found");
            }
            
            if (side == Order::buy) {
                bidOrders_.erase(std::get<Market::BidOrders::iterator>(it->second));
            } else {
                askOrders_.erase(std::get<Market::AskOrders::iterator>(it->second));
            }
            orderMap_.erase(it);
       }
    };

}

bool Market::insert(const Order& order) {
    if (orderMap_.count(order.getClOrdID())) {
        return false;
    }

    if (order.getSide() == Order::buy) {
        auto it = bidOrders_.insert(BidOrders::value_type(order.getPrice(), order));
        orderMap_.insert(std::make_pair(order.getClOrdID(), it));
    } else {
        auto it = askOrders_.insert(AskOrders::value_type(order.getPrice(), order));
        orderMap_.insert(std::make_pair(order.getClOrdID(), it));
    }
    return true;
}

void Market::erase(const Order& order) {
    ChooseHelper<OrderIteratorsSame, OrderMap>::erase(orderMap_, order.getSide(), bidOrders_, askOrders_, order.getClOrdID());
}

bool Market::match(std::queue<Order>& orders) {
    while (true) {
        if (bidOrders_.empty() || askOrders_.empty()) {
            return !orders.empty();
        }

        auto iBid = bidOrders_.begin();
        auto iAsk = askOrders_.begin();

        if (iBid->second.getPrice() >= iAsk->second.getPrice()) {
            Order& bid = iBid->second;
            Order& ask = iAsk->second;

            match(bid, ask);
            orders.push(bid);
            orders.push(ask);

            if (bid.isClosed()) {
                orderMap_.erase(bid.getClOrdID());
                bidOrders_.erase(iBid);
            }
            if (ask.isClosed()) {
                orderMap_.erase(ask.getClOrdID());
                askOrders_.erase(iAsk);
            }
        } else {
            return !orders.empty();
        }
    }
}

Order& Market::find(Order::Side side, const std::string& id) {
    return ChooseHelper<OrderIteratorsSame, OrderMap>::find(orderMap_, side, id);
}

void Market::match(Order& bid, Order& ask) {
    double price = ask.getPrice();
    uint32_t quantity = 0;

    if (bid.getOpenQuantity() > ask.getOpenQuantity()) {
        quantity = ask.getOpenQuantity();
    } else {
        quantity = bid.getOpenQuantity();
    }

    bid.execute(price, quantity);
    ask.execute(price, quantity);

    lastTradePrice_ = price;
    lastTradeSize_ = quantity;
    totalVolume_ += quantity;
}

bool Market::getBestBid(double& price, uint32_t& size) const {
    if (bidOrders_.empty()) return false;
    price = bidOrders_.begin()->first;
    size = 0;
    for (auto i = bidOrders_.begin(); i != bidOrders_.end() && i->first == price; ++i) {
        size += i->second.getOpenQuantity();
    }
    return true;
}

bool Market::getBestAsk(double& price, uint32_t& size) const {
    if (askOrders_.empty()) return false;
    price = askOrders_.begin()->first;
    size = 0;
    for (auto i = askOrders_.begin(); i != askOrders_.end() && i->first == price; ++i) {
        size += i->second.getOpenQuantity();
    }
    return true;
}

void Market::display() const {
    std::cout << "BIDS:" << std::endl;
    for (auto i = bidOrders_.begin(); i != bidOrders_.end(); ++i) {
        std::cout << "  ID: " << i->second.getClOrdID() << " Qty: " << i->second.getQuantity() << " Price: " << i->second.getPrice() << std::endl;
    }
    std::cout << "ASKS:" << std::endl;
    for (auto i = askOrders_.begin(); i != askOrders_.end(); ++i) {
        std::cout << "  ID: " << i->second.getClOrdID() << " Qty: " << i->second.getQuantity() << " Price: " << i->second.getPrice() << std::endl;
    }
}
