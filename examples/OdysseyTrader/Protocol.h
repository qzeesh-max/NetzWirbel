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

#pragma once
#include <cstdint>

enum class MsgType : uint16_t {
    LogonReq = 1,
    LogonResp = 2,
    NewOrder = 3,
    OrderReplace = 4,
    OrderCancel = 5,
    ExecReport = 6,
    CancelReject = 7,
    MDRequest = 8,
    MDUpdate = 9,
    LogonState = 10,
    Heartbeat = 11,
    MDReject = 12
};

struct alignas(16) MsgHeader {
    uint16_t type = 0;
    uint16_t schema = 0;
    uint32_t length = 0; // Length of payload following the header
};

template <typename T>
struct alignas(16) NetMsg {
    MsgHeader header;
    T payload;
};

struct LogonReqMsg {
    char username[32] = {0};
};

struct LogonRespMsg {
    uint8_t status = 0; // 0 = Success, 1 = Fail
    char session_name[64] = {0};
};

struct NewOrderMsg {
    uint8_t side = 0; // 1 = Buy, 2 = Sell
    uint8_t type = 0; // 1 = Market, 2 = Limit
    uint32_t qty = 0;
    double price = 0;
    char cl_ord_id[32] = {0};
    char symbol[16] = {0};
    char username[32] = {0};
};

struct OrderReplaceMsg {
    uint8_t side = 0;
    uint8_t type = 0;
    uint32_t qty = 0;
    double price = 0;
    char orig_cl_ord_id[32] = {0};
    char new_cl_ord_id[32] = {0};
    char symbol[16] = {0};
    char username[32] = {0};
};

struct OrderCancelMsg {
    uint8_t side = 0;
    char cl_ord_id[32] = {0};
    char orig_cl_ord_id[32] = {0};
    char symbol[16] = {0};
    char username[32] = {0};
};

struct ExecReportMsg {
    uint8_t status = 0; // '0'=New, '1'=Partially filled, '2'=Filled, '4'=Canceled, '8'=Rejected
    uint8_t side = 0; // 1 = Buy, 2 = Sell
    uint32_t qty = 0;
    uint32_t last_qty = 0;
    uint32_t cum_qty = 0;
    uint32_t leaves_qty = 0;
    double price = 0;
    double last_px = 0;
    char cl_ord_id[32] = {0};
    char orig_cl_ord_id[32] = {0};
    char order_id[32] = {0};
    char symbol[16] = {0};
    char text[64] = {0};
};

struct CancelRejectMsg {
    uint8_t side = 0;
    char cl_ord_id[32] = {0};
    char orig_cl_ord_id[32] = {0};
    char symbol[16] = {0};
    char reason[64] = {0};
};

struct MDRequestMsg {
    uint8_t sub_type = 0; // 0 = Subscribe, 1 = Unsubscribe
    char symbol[16] = {0};
};

struct MDRejectMsg {
    char symbol[16] = {0};
    char reason[64] = {0};
};

struct MDUpdateMsg {
    uint32_t bid_size = 0;
    uint32_t ask_size = 0;
    uint32_t last_size = 0;
    uint32_t total_volume = 0;
    double bid_px = 0;
    double ask_px = 0;
    double last_px = 0;
    char symbol[16];
};

struct alignas(16) LogonStateOrder {
    uint8_t side = 0;
    uint8_t status = 0;
    uint32_t qty = 0;
    uint32_t cum_qty = 0;
    double price = 0;
    char cl_ord_id[32] = {0};
    char symbol[16];
};

struct alignas(16) LogonStateExecution {
    uint8_t side = 0;
    uint32_t qty = 0;
    uint32_t exec_qty = 0;
    double price = 0;
    double exec_px = 0;
    char cl_ord_id[32] = {0};
    char symbol[16] = {0};
};

struct alignas(16) LogonStateReject {
    uint8_t side = 0;
    uint8_t is_cancel_replace_reject = 0; // 0 = Order Reject, 1 = Cancel Reject
    char cl_ord_id[32] = {0};
    char orig_cl_ord_id[32] = {0};
    char symbol[16] = {0};
    char reason[64] = {0};
};

struct alignas(16) LogonStateMsg {
    uint32_t order_count = 0;
    uint32_t execution_count = 0;
    uint32_t reject_count = 0;
    // Followed by order_count of LogonStateOrder, execution_count of LogonStateExecution, reject_count of LogonStateReject
};

