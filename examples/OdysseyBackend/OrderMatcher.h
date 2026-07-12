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
#include "Market.h"
#include <map>
#include <queue>
#include <stdexcept>

class OrderMatcher {
    typedef std::map<std::string, Market> Markets;

public:
    bool insert(const Order& order) {
        auto i = m_markets.find(order.getSymbol());
        if (i == m_markets.end()) {
            i = m_markets.insert(std::make_pair(order.getSymbol(), Market())).first;
        }
        return i->second.insert(order);
    }

    void erase(const Order& order) {
        auto i = m_markets.find(order.getSymbol());
        if (i == m_markets.end()) {
            return;
        }
        i->second.erase(order);
    }

    Order& find(const std::string& symbol, Order::Side side, const std::string& id) {
        auto i = m_markets.find(symbol);
        if (i == m_markets.end()) {
            throw std::runtime_error("Symbol not found");
        }
        return i->second.find(side, id);
    }

    bool match(const std::string& symbol, std::queue<Order>& orders) {
        auto i = m_markets.find(symbol);
        if (i == m_markets.end()) {
            return false;
        }
        return i->second.match(orders);
    }

    bool match(std::queue<Order>& orders) {
        for (auto i = m_markets.begin(); i != m_markets.end(); ++i) {
            i->second.match(orders);
        }
        return !orders.empty();
    }

    const Market* getMarket(const std::string& symbol) const {
        auto i = m_markets.find(symbol);
        if (i == m_markets.end()) return nullptr;
        return &i->second;
    }

    const Markets& getMarkets() const { return m_markets; }

private:
    Markets m_markets;
};
