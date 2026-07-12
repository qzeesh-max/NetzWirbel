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

#ifdef _MSC_VER
#pragma warning(disable : 4503 4355 4786)
#endif

#include "quickfix/config.h"

#include "Application.h"
#include "quickfix/Session.h"

#include "quickfix/fix42/ExecutionReport.h"
#include "quickfix/fix42/BusinessMessageReject.h"

void Application::onLogon(const FIX::SessionID &sessionID) {
  m_activeSessions.insert(sessionID);
}

void Application::onLogout(const FIX::SessionID &sessionID) {
  m_activeSessions.erase(sessionID);
}

void Application::fromApp(const FIX::Message &message,
                          const FIX::SessionID &sessionID)
    EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue,
           FIX::UnsupportedMessageType) {
  crack(message, sessionID);
}

void Application::onMessage(const FIX42::NewOrderSingle &message,
                            const FIX::SessionID &) {
  FIX::SenderCompID senderCompID;
  FIX::TargetCompID targetCompID;
  FIX::ClOrdID clOrdID;
  FIX::Symbol symbol;
  FIX::Side side;
  FIX::OrdType ordType;
  FIX::Price price;
  FIX::OrderQty orderQty;
  FIX::TimeInForce timeInForce(FIX::TimeInForce_DAY);

  message.getHeader().get(senderCompID);
  message.getHeader().get(targetCompID);
  message.get(clOrdID);
  message.get(symbol);
  message.get(side);
  message.get(ordType);
  if (ordType == FIX::OrdType_LIMIT) {
    message.get(price);
  }
  message.get(orderQty);
  message.getFieldIfSet(timeInForce);

  try {
    if (timeInForce != FIX::TimeInForce_DAY) {
      throw std::logic_error("Unsupported TIF, use Day");
    }

    Order order(clOrdID, symbol, senderCompID, targetCompID, convert(side),
                convert(ordType), price, (long)orderQty);

    processOrder(order);
  } catch (std::exception &e) {
    rejectOrder(senderCompID, targetCompID, clOrdID, symbol, side, e.what());
  }
}

void Application::onMessage(const FIX42::OrderCancelRequest &message,
                            const FIX::SessionID &sessionID) {
  FIX::OrigClOrdID origClOrdID;
  FIX::Symbol symbol;
  FIX::Side side;
  FIX::ClOrdID clOrdID;

  message.get(origClOrdID);
  message.get(symbol);
  message.get(side);
  message.get(clOrdID);

  try {
    processCancel(origClOrdID, symbol, convert(side));
  } catch (std::exception &e) {
    FIX42::OrderCancelReject reject(FIX::OrderID("NONE"), clOrdID, origClOrdID,
                                    FIX::OrdStatus_REJECTED,
                                    FIX::CxlRejResponseTo_ORDER_CANCEL_REQUEST);
    reject.set(FIX::Text(e.what()));
    FIX::Session::sendToTarget(reject, sessionID);
  }
}

void Application::onMessage(const FIX42::OrderCancelReplaceRequest &message,
                            const FIX::SessionID &sessionID) {
  FIX::OrigClOrdID origClOrdID;
  FIX::ClOrdID clOrdID;
  FIX::Symbol symbol;
  FIX::Side side;
  FIX::Price price;
  FIX::OrderQty orderQty;

  message.get(origClOrdID);
  message.get(clOrdID);
  message.get(symbol);
  message.get(side);
  message.get(price);
  message.get(orderQty);

  try {
    processReplace(origClOrdID, clOrdID, symbol, convert(side), price,
                   (long)orderQty);
  } catch (std::exception &e) {
    FIX42::OrderCancelReject reject(
        FIX::OrderID("NONE"), clOrdID, origClOrdID, FIX::OrdStatus_REJECTED,
        FIX::CxlRejResponseTo_ORDER_CANCEL_REPLACE_REQUEST);
    reject.set(FIX::Text(e.what()));
    FIX::Session::sendToTarget(reject, sessionID);
  }
}

void Application::onMessage(const FIX42::MarketDataRequest &message,
                            const FIX::SessionID &sessionID) {
  FIX::MDReqID mdReqID;
  FIX::SubscriptionRequestType subscriptionRequestType;
  FIX::MarketDepth marketDepth;
  FIX::NoRelatedSym noRelatedSym;
  FIX42::MarketDataRequest::NoRelatedSym noRelatedSymGroup;

  message.get(mdReqID);
  message.get(subscriptionRequestType);
  if (subscriptionRequestType != FIX::SubscriptionRequestType_SNAPSHOT) {
    throw FIX::IncorrectTagValue(subscriptionRequestType.getTag());
  }
  message.get(marketDepth);
  message.get(noRelatedSym);

  for (int i = 1; i <= noRelatedSym; ++i) {
    FIX::Symbol symbol;
    message.getGroup(i, noRelatedSymGroup);
    noRelatedSymGroup.get(symbol);
    std::string sym = symbol.getValue();
    if (sym == "LNUX" || sym == "YHOO" || sym == "CSCO") {
      broadcastMarketData(sym);
    } else {
      FIX::MsgSeqNum seqNum;
      message.getHeader().get(seqNum);
      FIX42::BusinessMessageReject reject(FIX::RefMsgType("V"), FIX::BusinessRejectReason_UNKNOWN_SECURITY);
      reject.set(FIX::RefSeqNum(seqNum));
      reject.set(FIX::Text("stock is not known"));
      try {
        FIX::Session::sendToTarget(reject, sessionID);
      } catch (FIX::SessionNotFound &) {}
    }
  }
}

void Application::onMessage(const FIX43::MarketDataRequest &message,
                            const FIX::SessionID &) {
  std::cout << message.toXML() << std::endl;
}

void Application::updateOrder(const Order &order, char status,
                              const std::string &origId) {
  FIX::TargetCompID targetCompID(order.getOwner());
  FIX::SenderCompID senderCompID(order.getTarget());

  FIX42::ExecutionReport fixOrder(FIX::OrderID(order.getClientID()),
                                  FIX::ExecID(m_generator.genExecutionID()),
                                  FIX::ExecTransType(FIX::ExecTransType_NEW),
                                  FIX::ExecType(status), FIX::OrdStatus(status),
                                  FIX::Symbol(order.getSymbol()),
                                  FIX::Side(convert(order.getSide())),
                                  FIX::LeavesQty(order.getOpenQuantity()),
                                  FIX::CumQty(order.getExecutedQuantity()),
                                  FIX::AvgPx(order.getAvgExecutedPrice()));

  if (!origId.empty()) {
    fixOrder.set(FIX::OrigClOrdID(origId));
  }

  fixOrder.set(FIX::ClOrdID(order.getClientID()));
  fixOrder.set(FIX::OrderQty(order.getQuantity()));

  if (status == FIX::OrdStatus_FILLED ||
      status == FIX::OrdStatus_PARTIALLY_FILLED) {
    fixOrder.set(FIX::LastShares(order.getLastExecutedQuantity()));
    fixOrder.set(FIX::LastPx(order.getLastExecutedPrice()));
  }

  try {
    FIX::Session::sendToTarget(fixOrder, senderCompID, targetCompID);
  } catch (FIX::SessionNotFound &) {
  }
}

void Application::rejectOrder(const FIX::SenderCompID &sender,
                              const FIX::TargetCompID &target,
                              const FIX::ClOrdID &clOrdID,
                              const FIX::Symbol &symbol, const FIX::Side &side,
                              const std::string &message) {
  FIX::TargetCompID targetCompID(sender.getValue());
  FIX::SenderCompID senderCompID(target.getValue());

  FIX42::ExecutionReport fixOrder(FIX::OrderID(clOrdID.getValue()),
                                  FIX::ExecID(m_generator.genExecutionID()),
                                  FIX::ExecTransType(FIX::ExecTransType_NEW),
                                  FIX::ExecType(FIX::ExecType_REJECTED),
                                  FIX::OrdStatus(FIX::ExecType_REJECTED),
                                  symbol, side, FIX::LeavesQty(0),
                                  FIX::CumQty(0), FIX::AvgPx(0));

  fixOrder.set(clOrdID);
  fixOrder.set(FIX::Text(message));

  try {
    FIX::Session::sendToTarget(fixOrder, senderCompID, targetCompID);
  } catch (FIX::SessionNotFound &) {
  }
}

void Application::processOrder(const Order &order) {
  if (m_orderMatcher.insert(order)) {
    acceptOrder(order);

    std::queue<Order> orders;
    m_orderMatcher.match(order.getSymbol(), orders);

    while (orders.size()) {
      fillOrder(orders.front());
      orders.pop();
    }

    broadcastMarketData(order.getSymbol());
  } else {
    rejectOrder(order);
  }
}

void Application::processCancel(const std::string &id,
                                const std::string &symbol, Order::Side side) {
  Order &order = m_orderMatcher.find(symbol, side, id);
  order.cancel();
  cancelOrder(order);
  m_orderMatcher.erase(order);
  broadcastMarketData(symbol);
}

void Application::processReplace(const std::string &origId,
                                 const std::string &newId,
                                 const std::string &symbol, Order::Side side,
                                 double price, long qty) {
  Order order = m_orderMatcher.find(symbol, side, origId);
  m_orderMatcher.erase(order);
  order.replace(newId, price, qty);

  if (m_orderMatcher.insert(order)) {
    // Send ExecutionReport (Replaced)
    updateOrder(order, FIX::OrdStatus_REPLACED, origId);

    std::queue<Order> orders;
    m_orderMatcher.match(symbol, orders);

    while (orders.size()) {
      fillOrder(orders.front());
      orders.pop();
    }

    broadcastMarketData(symbol);
  } else {
    rejectOrder(order);
  }
}

Order::Side Application::convert(const FIX::Side &side) {
  switch (side) {
  case FIX::Side_BUY:
    return Order::buy;
  case FIX::Side_SELL:
    return Order::sell;
  default:
    throw std::logic_error("Unsupported Side, use buy or sell");
  }
}

Order::Type Application::convert(const FIX::OrdType &ordType) {
  switch (ordType) {
  case FIX::OrdType_LIMIT:
    return Order::limit;
  default:
    throw std::logic_error("Unsupported Order Type, use limit");
  }
}

FIX::Side Application::convert(Order::Side side) {
  switch (side) {
  case Order::buy:
    return FIX::Side(FIX::Side_BUY);
  case Order::sell:
    return FIX::Side(FIX::Side_SELL);
  default:
    throw std::logic_error("Unsupported Side, use buy or sell");
  }
}

FIX::OrdType Application::convert(Order::Type type) {
  switch (type) {
  case Order::limit:
    return FIX::OrdType(FIX::OrdType_LIMIT);
  default:
    throw std::logic_error("Unsupported Order Type, use limit");
  }
}

#include "quickfix/fix42/MarketDataSnapshotFullRefresh.h"
void Application::broadcastMarketData(const std::string &symbol) {
  if (m_activeSessions.empty())
    return;

  const Market *market = m_orderMatcher.getMarket(symbol);

  FIX42::MarketDataSnapshotFullRefresh snapshot;
  snapshot.set(FIX::Symbol(symbol));

  double bestBidPx = 0, bestAskPx = 0;
  long bestBidSz = 0, bestAskSz = 0;

  int numEntries = 0;

  if (market) {
    bool hasBid = market->getBestBid(bestBidPx, bestBidSz);
    bool hasAsk = market->getBestAsk(bestAskPx, bestAskSz);

    if (hasBid) {
      FIX42::MarketDataSnapshotFullRefresh::NoMDEntries group;
      group.set(FIX::MDEntryType(FIX::MDEntryType_BID));
      group.set(FIX::MDEntryPx(bestBidPx));
      group.set(FIX::MDEntrySize(bestBidSz));
      snapshot.addGroup(group);
      numEntries++;
    }

    if (hasAsk) {
      FIX42::MarketDataSnapshotFullRefresh::NoMDEntries group;
      group.set(FIX::MDEntryType(FIX::MDEntryType_OFFER));
      group.set(FIX::MDEntryPx(bestAskPx));
      group.set(FIX::MDEntrySize(bestAskSz));
      snapshot.addGroup(group);
      numEntries++;
    }

    if (market->getLastTradeSize() > 0) {
      FIX42::MarketDataSnapshotFullRefresh::NoMDEntries group;
      group.set(FIX::MDEntryType(FIX::MDEntryType_TRADE));
      group.set(FIX::MDEntryPx(market->getLastTradePrice()));
      group.set(FIX::MDEntrySize(market->getLastTradeSize()));
      snapshot.addGroup(group);
      numEntries++;
    }

    if (market->getTotalVolume() > 0) {
      FIX42::MarketDataSnapshotFullRefresh::NoMDEntries group;
      group.set(FIX::MDEntryType(FIX::MDEntryType_TRADE_VOLUME));
      group.set(FIX::MDEntryPx(0));
      group.set(FIX::MDEntrySize(market->getTotalVolume()));
      snapshot.addGroup(group);
      numEntries++;
    }
  }

  snapshot.set(FIX::NoMDEntries(numEntries));

  for (const auto &sid : m_activeSessions) {
    try {
      FIX::Session::sendToTarget(snapshot, sid);
    } catch (FIX::SessionNotFound &) {
    }
  }
}
