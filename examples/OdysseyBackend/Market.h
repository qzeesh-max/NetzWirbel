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
#include "Order.h"
#include <map>
#include <queue>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <type_traits>

class Market {
public:
    using BidOrders = std::multimap<double, Order, std::greater<double>>;
    using AskOrders = std::multimap<double, Order, std::less<double>>;
    static constexpr const bool OrderIteratorsSame = std::is_same_v<BidOrders::iterator, AskOrders::iterator>;
    using OrderMapValueType = std::conditional_t<OrderIteratorsSame,  BidOrders::iterator, std::variant<BidOrders::iterator, AskOrders::iterator>>;
    using OrderMap = std::unordered_map<std::string, OrderMapValueType>;

    bool insert(const Order& order);
    void erase(const Order& order);
    Order& find(Order::Side side, const std::string& id);
    bool match(std::queue<Order>&);
    void display() const;

    double getLastTradePrice() const { return lastTradePrice_; }
    uint32_t getLastTradeSize() const { return lastTradeSize_; }
    uint32_t getTotalVolume() const { return totalVolume_; }
    bool getBestBid(double& price, uint32_t& size) const;
    bool getBestAsk(double& price, uint32_t& size) const;

private:
    
    void match(Order& bid, Order& ask);

    std::queue<Order> orderUpdates_;
    BidOrders bidOrders_;
    AskOrders askOrders_;
  
    OrderMap orderMap_;
    
    double lastTradePrice_ = 0.0;
    uint32_t lastTradeSize_ = 0;
    uint32_t totalVolume_ = 0;
};
