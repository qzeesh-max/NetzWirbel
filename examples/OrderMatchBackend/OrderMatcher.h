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
 *
 * This product includes software developed by quickfixengine.org (http://www.quickfixengine.org/).
 */

/* -*- C++ -*- */

/****************************************************************************
** Copyright (c) 2001-2014
**
** This file is part of the QuickFIX FIX Engine
**
** This file may be distributed under the terms of the quickfixengine.org
** license as defined by quickfixengine.org and appearing in the file
** LICENSE included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.quickfixengine.org/LICENSE for licensing information.
**
** Contact ask@quickfixengine.org if any conditions of this licensing are
** not clear to you.
**
****************************************************************************/

#ifndef ORDERMATCH_ORDERMATCHER_H
#define ORDERMATCH_ORDERMATCHER_H

#include "Market.h"
#include <iostream>
#include <map>

class OrderMatcher {
  typedef std::map<std::string, Market> Markets;

public:
  bool insert(const Order &order) {
    Markets::iterator i = m_markets.find(order.getSymbol());
    if (i == m_markets.end()) {
      i = m_markets.insert(std::make_pair(order.getSymbol(), Market())).first;
    }
    return i->second.insert(order);
  }

  void erase(const Order &order) {
    Markets::iterator i = m_markets.find(order.getSymbol());
    if (i == m_markets.end()) {
      return;
    }
    i->second.erase(order);
  }

  Order &find(std::string symbol, Order::Side side, std::string id) {
    Markets::iterator i = m_markets.find(symbol);
    if (i == m_markets.end()) {
      throw std::runtime_error("Symbol not found");
    }
    return i->second.find(side, id);
  }

  bool match(std::string symbol, std::queue<Order> &orders) {
    Markets::iterator i = m_markets.find(symbol);
    if (i == m_markets.end()) {
      return false;
    }
    return i->second.match(orders);
  }

  bool match(std::queue<Order> &orders) {
    Markets::iterator i;
    for (i = m_markets.begin(); i != m_markets.end(); ++i) {
      i->second.match(orders);
    }
    return orders.size() != 0;
  }

  void display(std::string symbol) const {
    Markets::const_iterator i = m_markets.find(symbol);
    if (i == m_markets.end()) {
      return;
    }
    i->second.display();
  }

  void display() const {
    std::cout << "SYMBOLS:" << std::endl;
    std::cout << "--------" << std::endl;

    Markets::const_iterator i;
    for (i = m_markets.begin(); i != m_markets.end(); ++i) {
      std::cout << i->first << std::endl;
    }
  }

  const Market* getMarket(const std::string& symbol) const {
    Markets::const_iterator i = m_markets.find(symbol);
    if (i == m_markets.end()) return nullptr;
    return &i->second;
  }
  
  const Markets& getMarkets() const { return m_markets; }

private:
  Markets m_markets;
};

#endif
