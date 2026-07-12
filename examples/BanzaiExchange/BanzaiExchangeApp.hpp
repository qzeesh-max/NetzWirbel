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

#pragma once
#include "NetzWirbel/App.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include "FixParser.h"
#include <emscripten.h>
#include <emscripten/websocket.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace NetzWirbel;
using namespace std;

struct MarketData {
  string symbol;
  double bidPx = 0, askPx = 0, lastPx = 0;
  long bidSz = 0, askSz = 0, lastSz = 0, volume = 0;
};

struct OrderData {
  string id, origId, symbol, side, type, status;
  double price = 0, avgPx = 0;
  long qty = 0, leaves = 0, cumQty = 0;
};

struct RejectData {
  string id, origId, refMsgType, status, text;
};

struct ExecutionData {
  string id, execId, symbol, side, execType;
  double lastPx = 0, avgPx = 0;
  long lastSz = 0, cumQty = 0;
};

class BanzaiExchangeApp;
extern BanzaiExchangeApp *g_app_instance;

class BanzaiExchangeApp : public App {
private:
  std::string generateOrderId() {
      unsigned long a = lrand48() & 0xFFFF;
      unsigned long b = lrand48() & 0xFFFF;
      unsigned long c = lrand48() & 0xFFFF;
      char buf[64];
      snprintf(buf, sizeof(buf), "%04lx-%04lx-%04lx", a, b, c);
      return std::string(buf);
  }

public:
  ~BanzaiExchangeApp() {
    if (ws_ > 0) {
      emscripten_websocket_delete(ws_);
      ws_ = 0;
    }
  }

  void on_init(Context *ctx) override {
    ctx_ = ctx;
    g_app_instance = this;

    main_div = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(main_div);
    Command cmd;
    cmd.type = CommandType::APPEND_CHILD;
    cmd.target_id = 0;
    cmd.arg1 = main_div->get_id();
    ctx->send_command(cmd);

    main_div->set_attribute(ctx->strings.style,
                            "font-family: sans-serif; padding: 20px;");

    auto header = std::make_shared<Element>(ctx, ctx->strings.h1);
    ctx->register_element(header);
    header->set_text_content(ctx->register_string("Banzai FIX Exchange"));
    main_div->append_child(header);

    // Stylish Status Pane at the top
    status_bar = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(status_bar);
    status_bar->set_attribute(ctx->strings.style,
                              "display: flex; justify-content: space-between; align-items: center; "
                              "background: #1e1e1e; color: #ccc; padding: 10px 15px; border-radius: 6px; "
                              "margin-bottom: 20px; font-family: monospace; font-size: 13px; border: 1px solid #333;");
    main_div->append_child(status_bar);

    auto status_left = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(status_left);
    status_left->set_attribute(ctx->strings.style, "display: flex; align-items: center;");
    status_bar->append_child(status_left);

    status_light = std::make_shared<Element>(ctx, ctx->register_string("span"));
    ctx->register_element(status_light);
    status_left->append_child(status_light);

    status_text = std::make_shared<Element>(ctx, ctx->register_string("span"));
    ctx->register_element(status_text);
    status_left->append_child(status_text);

    seq_num_text = std::make_shared<Element>(ctx, ctx->register_string("span"));
    ctx->register_element(seq_num_text);
    seq_num_text->set_attribute(ctx->strings.style, "font-weight: bold; color: #4fc3f7;");
    status_bar->append_child(seq_num_text);

    updateStatusBar();

    // Top Split: Market Data + Order Entry
    auto topCont = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(topCont);
    topCont->set_attribute(ctx->strings.style,
                           "display: flex; gap: 20px; margin-bottom: 20px;");
    main_div->append_child(topCont);

    // Market Data Container
    buildMarketData(topCont);

    // Order Entry Container
    buildOrderEntry(topCont);

    // Tabs Container
    auto tabCont = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(tabCont);
    tabCont->set_attribute(
        ctx->strings.style,
        "margin-top: 20px; border: 1px solid #ccc; border-radius: 4px;");
    main_div->append_child(tabCont);

    auto tabHeader = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(tabHeader);
    tabHeader->set_attribute(
        ctx->strings.style,
        "display: flex; background: #eee; border-bottom: 1px solid #ccc;");
    tabCont->append_child(tabHeader);

    auto btnOrders = std::make_shared<HTMLButtonElement>(ctx);
    ctx->register_element(btnOrders);
    btnOrders->set_text_content(ctx->register_string("Active Orders"));
    btnOrders->set_attribute(
        ctx->strings.style,
        "padding: 10px; cursor: pointer; border: none; background: #fff; "
        "font-weight: bold; border-right: 1px solid #ccc;");

    auto btnTrades = std::make_shared<HTMLButtonElement>(ctx);
    ctx->register_element(btnTrades);
    btnTrades->set_text_content(ctx->register_string("Trades"));
    btnTrades->set_attribute(
        ctx->strings.style,
        "padding: 10px; cursor: pointer; border: none; background: "
        "transparent; border-right: 1px solid #ccc;");

    auto btnRejects = std::make_shared<HTMLButtonElement>(ctx);
    ctx->register_element(btnRejects);
    btnRejects->set_text_content(ctx->register_string("Rejects"));
    btnRejects->set_attribute(ctx->strings.style,
                              "padding: 10px; cursor: pointer; border: none; "
                              "background: transparent;");

    tabHeader->append_child(btnOrders);
    tabHeader->append_child(btnTrades);
    tabHeader->append_child(btnRejects);

    tabOrders = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(tabOrders);
    tabOrders->set_attribute(ctx->strings.style,
                             "padding: 15px; display: block;");
    buildOrdersTab(tabOrders);

    tabTrades = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(tabTrades);
    tabTrades->set_attribute(ctx->strings.style,
                             "padding: 15px; display: none;");
    buildTradesTab(tabTrades);

    tabRejects = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(tabRejects);
    tabRejects->set_attribute(ctx->strings.style,
                              "padding: 15px; display: none;");
    buildRejectsTab(tabRejects);

    tabCont->append_child(tabOrders);
    tabCont->append_child(tabTrades);
    tabCont->append_child(tabRejects);

    // Tab Switching Logic
    btnOrders->add_event_listener(
        ctx->strings.click,
        [this, btnOrders, btnTrades, btnRejects](const Event &e) {
          tabOrders->set_attribute(ctx_->strings.style,
                                   "padding: 15px; display: block;");
          tabTrades->set_attribute(ctx_->strings.style,
                                   "padding: 15px; display: none;");
          tabRejects->set_attribute(ctx_->strings.style,
                                    "padding: 15px; display: none;");
          btnOrders->set_attribute(
              ctx_->strings.style,
              "padding: 10px; cursor: pointer; border: none; background: #fff; "
              "font-weight: bold; border-right: 1px solid #ccc;");
          btnTrades->set_attribute(
              ctx_->strings.style,
              "padding: 10px; cursor: pointer; border: none; background: "
              "transparent; border-right: 1px solid #ccc;");
          btnRejects->set_attribute(ctx_->strings.style,
                                    "padding: 10px; cursor: pointer; border: "
                                    "none; background: transparent;");
        });
    btnTrades->add_event_listener(
        ctx->strings.click,
        [this, btnOrders, btnTrades, btnRejects](const Event &e) {
          tabOrders->set_attribute(ctx_->strings.style,
                                   "padding: 15px; display: none;");
          tabTrades->set_attribute(ctx_->strings.style,
                                   "padding: 15px; display: block;");
          tabRejects->set_attribute(ctx_->strings.style,
                                    "padding: 15px; display: none;");
          btnOrders->set_attribute(
              ctx_->strings.style,
              "padding: 10px; cursor: pointer; border: none; background: "
              "transparent; border-right: 1px solid #ccc;");
          btnTrades->set_attribute(
              ctx_->strings.style,
              "padding: 10px; cursor: pointer; border: none; background: #fff; "
              "font-weight: bold; border-right: 1px solid #ccc;");
          btnRejects->set_attribute(ctx_->strings.style,
                                    "padding: 10px; cursor: pointer; border: "
                                    "none; background: transparent;");
        });
    btnRejects->add_event_listener(ctx->strings.click, [this, btnOrders,
                                                        btnTrades, btnRejects](
                                                           const Event &e) {
      tabOrders->set_attribute(ctx_->strings.style,
                               "padding: 15px; display: none;");
      tabTrades->set_attribute(ctx_->strings.style,
                               "padding: 15px; display: none;");
      tabRejects->set_attribute(ctx_->strings.style,
                                "padding: 15px; display: block;");
      btnOrders->set_attribute(
          ctx_->strings.style,
          "padding: 10px; cursor: pointer; border: none; background: "
          "transparent; border-right: 1px solid #ccc;");
      btnTrades->set_attribute(
          ctx_->strings.style,
          "padding: 10px; cursor: pointer; border: none; background: "
          "transparent; border-right: 1px solid #ccc;");
      btnRejects->set_attribute(ctx_->strings.style,
                                "padding: 10px; cursor: pointer; border: none; "
                                "background: #fff; font-weight: bold;");
    });

    // WebSocket Connect
    connectWebSocket();
  }

  void buildMarketData(shared_ptr<Element> parent) {
    auto mdCont = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(mdCont);
    mdCont->set_attribute(ctx_->strings.style, "flex: 1;");
    parent->append_child(mdCont);

    auto mdHeader = std::make_shared<Element>(ctx_, ctx_->strings.h3);
    ctx_->register_element(mdHeader);
    mdHeader->set_text_content(ctx_->register_string("Market Data"));
    mdCont->append_child(mdHeader);

    error_alert = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(error_alert);
    error_alert->set_attribute(ctx_->strings.style,
                               "display: none; background-color: #ff3b30; color: #ffcc00; "
                               "font-weight: bold; padding: 10px; border-radius: 4px; "
                               "margin-bottom: 10px; text-align: center; font-size: 14px;");
    mdCont->append_child(error_alert);

    auto controls = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(controls);
    controls->set_attribute(ctx_->strings.style,
                            "margin-bottom: 10px; display: flex; gap: 10px;");
    mdCont->append_child(controls);

    auto newSymIn = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(newSymIn);
    newSymIn->set_attribute(ctx_->strings.placeholder, "Add Symbol...");
    // Listen to change to populate value property correctly
    controls->append_child(newSymIn);

    auto addBtn = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(addBtn);
    addBtn->set_text_content(ctx_->register_string("Add"));
    addBtn->add_event_listener(ctx_->strings.click,
                               [this, newSymIn](const Event &e) {
                                 string sym = newSymIn->get_value();
                                 if (!sym.empty()) {
                                   requestMarketData(sym);
                                   newSymIn->set_value("");
                                 }
                               });
    controls->append_child(addBtn);

    md_table = std::make_shared<Element>(ctx_, ctx_->strings.table);
    ctx_->register_element(md_table);
    md_table->set_attribute(ctx_->strings.style,
                            "width: 100%; border-collapse: collapse;");
    mdCont->append_child(md_table);

    auto mdTrh = std::make_shared<Element>(ctx_, ctx_->strings.tr);
    ctx_->register_element(mdTrh);
    vector<string> mdCols = {"Symbol", "Bid Sz",  "Bid Px",  "Ask Px",
                             "Ask Sz", "Last Px", "Last Sz", "Volume"};
    for (const auto &c : mdCols) {
      auto th = std::make_shared<Element>(ctx_, ctx_->strings.th);
      ctx_->register_element(th);
      th->set_attribute(
          ctx_->strings.style,
          "border: 1px solid #ccc; padding: 5px; background: #eee;");
      th->set_text_content(ctx_->register_string(c));
      mdTrh->append_child(th);
    }
    md_table->append_child(mdTrh);
  }

  void buildOrderEntry(shared_ptr<Element> parent) {
    auto oeCont = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(oeCont);
    oeCont->set_attribute(ctx_->strings.style,
                          "width: 300px; padding: 15px; border: 1px solid "
                          "#ccc; background: #fafafa;");
    parent->append_child(oeCont);

    auto oeHeader = std::make_shared<Element>(ctx_, ctx_->strings.h3);
    ctx_->register_element(oeHeader);
    oeHeader->set_text_content(ctx_->register_string("Order Entry"));
    oeCont->append_child(oeHeader);

    sym_in = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(sym_in);
    sym_in->set_attribute(ctx_->strings.placeholder, "Symbol");
    sym_in->set_attribute(ctx_->strings.style,
                          "width: 100%; margin-bottom: 10px; padding: 5px; "
                          "box-sizing: border-box;");
    oeCont->append_child(sym_in);

    qty_in = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(qty_in);
    qty_in->set_attribute(ctx_->strings.type, "number");
    qty_in->set_attribute(ctx_->strings.placeholder, "Quantity");
    qty_in->set_attribute(ctx_->strings.style,
                          "width: 100%; margin-bottom: 10px; padding: 5px; "
                          "box-sizing: border-box;");
    oeCont->append_child(qty_in);

    px_in = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(px_in);
    px_in->set_attribute(ctx_->strings.type, "number");
    px_in->set_attribute(ctx_->strings.placeholder, "Price");
    px_in->set_attribute(ctx_->strings.style,
                         "width: 100%; margin-bottom: 10px; padding: 5px; "
                         "box-sizing: border-box;");
    oeCont->append_child(px_in);

    side_sel = std::make_shared<HTMLSelectElement>(ctx_);
    ctx_->register_element(side_sel);
    side_sel->set_attribute(ctx_->strings.style,
                            "width: 100%; margin-bottom: 15px; padding: 5px; "
                            "box-sizing: border-box;");

    auto optBuy = std::make_shared<HTMLOptionElement>(ctx_);
    ctx_->register_element(optBuy);
    optBuy->set_attribute(ctx_->strings.value, "1");
    optBuy->set_text_content(ctx_->register_string("Buy"));
    auto optSell = std::make_shared<HTMLOptionElement>(ctx_);
    ctx_->register_element(optSell);
    optSell->set_attribute(ctx_->strings.value, "2");
    optSell->set_text_content(ctx_->register_string("Sell"));
    side_sel->append_child(optBuy);
    side_sel->append_child(optSell);

    // Listen to select change so get_value works
    side_sel->set_value("1");

    oeCont->append_child(side_sel);

    auto btn = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(btn);
    btn->set_text_content(ctx_->register_string("Send Order"));
    btn->set_attribute(
        ctx_->strings.style,
        "width: 100%; padding: 8px; background: #007bff; color: white; border: "
        "none; cursor: pointer; box-sizing: border-box;");
    btn->add_event_listener(ctx_->strings.click,
                            [this](const Event &e) { this->sendNewOrder(); });
    oeCont->append_child(btn);
  }

  void buildOrdersTab(shared_ptr<Element> parent) {
    // Global Actions
    auto gActions = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(gActions);
    gActions->set_attribute(ctx_->strings.style,
                            "margin-bottom: 10px; display: flex; gap: 10px;");

    auto btnCxB = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(btnCxB);
    btnCxB->set_text_content(ctx_->register_string("Cancel Buys"));
    btnCxB->set_attribute(ctx_->strings.style,
                          "padding: 5px 10px; cursor: pointer;");
    btnCxB->add_event_listener(ctx_->strings.click, [this](const Event &) {
      this->cancelOrdersBySide("Buy");
    });

    auto btnCxS = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(btnCxS);
    btnCxS->set_text_content(ctx_->register_string("Cancel Sells"));
    btnCxS->set_attribute(ctx_->strings.style,
                          "padding: 5px 10px; cursor: pointer;");
    btnCxS->add_event_listener(ctx_->strings.click, [this](const Event &) {
      this->cancelOrdersBySide("Sell");
    });

    auto btnCxA = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(btnCxA);
    btnCxA->set_text_content(ctx_->register_string("Cancel All"));
    btnCxA->set_attribute(ctx_->strings.style,
                          "padding: 5px 10px; cursor: pointer; background: "
                          "#dc3545; color: white; border: 1px solid #c82333;");
    btnCxA->add_event_listener(ctx_->strings.click, [this](const Event &) {
      this->cancelOrdersBySide("");
    });

    gActions->append_child(btnCxB);
    gActions->append_child(btnCxS);
    gActions->append_child(btnCxA);
    parent->append_child(gActions);

    // Split layout: Table on left, Details/Modify on right
    auto split = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(split);
    split->set_attribute(ctx_->strings.style, "display: flex; gap: 20px;");
    parent->append_child(split);

    auto tblCont = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(tblCont);
    tblCont->set_attribute(ctx_->strings.style, "flex: 1;");
    split->append_child(tblCont);

    ord_table = std::make_shared<Element>(ctx_, ctx_->strings.table);
    ctx_->register_element(ord_table);
    ord_table->set_attribute(ctx_->strings.style,
                             "width: 100%; border-collapse: collapse;");
    tblCont->append_child(ord_table);

    auto ordTrh = std::make_shared<Element>(ctx_, ctx_->strings.tr);
    ctx_->register_element(ordTrh);
    vector<string> ordCols = {"ID",    "Symbol", "Side",  "Qty",
                              "Price", "Leaves", "AvgPx", "Status"};
    for (const auto &c : ordCols) {
      auto th = std::make_shared<Element>(ctx_, ctx_->strings.th);
      ctx_->register_element(th);
      th->set_attribute(ctx_->strings.style,
                        "border: 1px solid #ccc; padding: 5px; background: "
                        "#eee; text-align: left;");
      th->set_text_content(ctx_->register_string(c));
      ordTrh->append_child(th);
    }
    ord_table->append_child(ordTrh);

    // Edit Form
    auto editCont = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(editCont);
    editCont->set_attribute(ctx_->strings.style,
                            "width: 250px; padding: 15px; border: 1px solid "
                            "#ccc; background: #fafafa;");
    split->append_child(editCont);

    auto editHeader = std::make_shared<Element>(ctx_, ctx_->strings.h4);
    ctx_->register_element(editHeader);
    editHeader->set_text_content(ctx_->register_string("Selected Order"));
    editCont->append_child(editHeader);

    selOrdIdDisp = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(selOrdIdDisp);
    selOrdIdDisp->set_attribute(
        ctx_->strings.style,
        "margin-bottom: 10px; font-weight: bold; color: #555;");
    selOrdIdDisp->set_text_content(ctx_->register_string("None"));
    editCont->append_child(selOrdIdDisp);

    repQtyIn = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(repQtyIn);
    repQtyIn->set_attribute(ctx_->strings.type, "number");
    repQtyIn->set_attribute(ctx_->strings.placeholder, "New Qty");
    repQtyIn->set_attribute(ctx_->strings.style,
                            "width: 100%; margin-bottom: 10px; padding: 5px; "
                            "box-sizing: border-box;");
    editCont->append_child(repQtyIn);

    repPxIn = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(repPxIn);
    repPxIn->set_attribute(ctx_->strings.type, "number");
    repPxIn->set_attribute(ctx_->strings.placeholder, "New Price");
    repPxIn->set_attribute(ctx_->strings.style,
                           "width: 100%; margin-bottom: 10px; padding: 5px; "
                           "box-sizing: border-box;");
    editCont->append_child(repPxIn);

    auto btnRep = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(btnRep);
    btnRep->set_text_content(ctx_->register_string("Replace Order"));
    btnRep->set_attribute(
        ctx_->strings.style,
        "width: 100%; margin-bottom: 10px; padding: 5px; background: #28a745; "
        "color: white; border: none; cursor: pointer; box-sizing: border-box;");
    btnRep->add_event_listener(ctx_->strings.click, [this](const Event &) {
      this->replaceSelected();
    });
    editCont->append_child(btnRep);

    auto btnCxl = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(btnCxl);
    btnCxl->set_text_content(ctx_->register_string("Cancel Order"));
    btnCxl->set_attribute(
        ctx_->strings.style,
        "width: 100%; padding: 5px; background: #dc3545; color: white; border: "
        "none; cursor: pointer; box-sizing: border-box;");
    btnCxl->add_event_listener(
        ctx_->strings.click, [this](const Event &) { this->cancelSelected(); });
    editCont->append_child(btnCxl);
  }

  void buildTradesTab(shared_ptr<Element> parent) {
    trd_table = std::make_shared<Element>(ctx_, ctx_->strings.table);
    ctx_->register_element(trd_table);
    trd_table->set_attribute(ctx_->strings.style,
                             "width: 100%; border-collapse: collapse;");
    parent->append_child(trd_table);

    auto thR = std::make_shared<Element>(ctx_, ctx_->strings.tr);
    ctx_->register_element(thR);
    vector<string> cols = {"ExecID",  "ClOrdID", "Symbol", "Side",
                           "Last Px", "Last Sz", "CumQty", "Type"};
    for (const auto &c : cols) {
      auto th = std::make_shared<Element>(ctx_, ctx_->strings.th);
      ctx_->register_element(th);
      th->set_attribute(ctx_->strings.style,
                        "border: 1px solid #ccc; padding: 5px; background: "
                        "#eee; text-align: left;");
      th->set_text_content(ctx_->register_string(c));
      thR->append_child(th);
    }
    trd_table->append_child(thR);
  }

  void buildRejectsTab(shared_ptr<Element> parent) {
    rej_table = std::make_shared<Element>(ctx_, ctx_->strings.table);
    ctx_->register_element(rej_table);
    rej_table->set_attribute(ctx_->strings.style,
                             "width: 100%; border-collapse: collapse;");
    parent->append_child(rej_table);

    auto thR = std::make_shared<Element>(ctx_, ctx_->strings.tr);
    ctx_->register_element(thR);
    vector<string> cols = {"ClOrdID", "OrigClOrdID", "RefMsg", "Status",
                           "Reason"};
    for (const auto &c : cols) {
      auto th = std::make_shared<Element>(ctx_, ctx_->strings.th);
      ctx_->register_element(th);
      th->set_attribute(ctx_->strings.style,
                        "border: 1px solid #ccc; padding: 5px; background: "
                        "#eee; text-align: left;");
      th->set_text_content(ctx_->register_string(c));
      thR->append_child(th);
    }
    rej_table->append_child(thR);
  }

  void sendFixMessage(const FixMessage &msg) {
    if (ws_) {
      if (msg.msgType != FIX::MsgType_Logon && sessionState_ != LOGGED_ON) {
        return;
      }
      int seqNum = fixSeqNum_++;
      senderSeqNum_ = seqNum;
      std::string raw = msg.toString(seqNum, "BANZAI", "ORDERMATCH");
      emscripten_websocket_send_utf8_text(ws_, raw.c_str());
      lastSentTime_ = emscripten_get_now();
      updateStatusBar();
    }
  }

  void sendHeartbeat(const string &testReqId = "") {
    FixMessage hb(FIX::MsgType_Heartbeat);
    if (!testReqId.empty()) {
      hb.setField(FIX::FIELD::TestReqID, testReqId);
    }
    sendFixMessage(hb);
  }

  void showStockNotKnownAlert() {
    error_alert->set_text_content(ctx_->register_string("stock is not known"));
    error_alert->set_attribute(ctx_->strings.style,
                               "display: block; background-color: #ff3b30; color: #ffcc00; "
                               "font-weight: bold; padding: 10px; border-radius: 4px; "
                               "margin-bottom: 10px; text-align: center; font-size: 14px; "
                               "opacity: 1.0;");
    alertStartTime_ = emscripten_get_now();
    alertActive_ = true;
  }

  void on_tick(double time) override {
    // Reconnect logic
    if (!ws_) {
      if (lastConnectAttemptTime_ == 0.0) {
        lastConnectAttemptTime_ = time;
      }
      if (time - lastConnectAttemptTime_ >= 15000.0) { // 15 seconds
        connectWebSocket();
      }
    } else if (sessionState_ == CONNECTED) {
      if (logonSentTime_ > 0.0 && time - logonSentTime_ >= 5000.0) {
        connectWebSocket();
      }
    }

    // FIX Heartbeat logic
    if (ws_ && sessionState_ == LOGGED_ON) {
      if (lastSentTime_ == 0.0) {
        lastSentTime_ = time;
      }
      if (time - lastSentTime_ >= 30000.0) { // 30 seconds
        sendHeartbeat();
      }
    }

    // Alert fade-out animation logic
    if (alertActive_) {
      double elapsed = time - alertStartTime_;
      if (elapsed > 1000.0 && elapsed <= 3000.0) {
        double opacity = 1.0 - (elapsed - 1000.0) / 2000.0;
        error_alert->set_attribute(ctx_->strings.style,
                                   "display: block; background-color: #ff3b30; color: #ffcc00; "
                                   "font-weight: bold; padding: 10px; border-radius: 4px; "
                                   "margin-bottom: 10px; text-align: center; font-size: 14px; "
                                   "opacity: " + std::to_string(opacity) + ";");
      } else if (elapsed > 3000.0) {
        error_alert->set_attribute(ctx_->strings.style,
                                   "display: none; background-color: #ff3b30; color: #ffcc00; "
                                   "font-weight: bold; padding: 10px; border-radius: 4px; "
                                   "margin-bottom: 10px; text-align: center; font-size: 14px; "
                                   "opacity: 0.0;");
        alertActive_ = false;
      }
    }
  }

  void requestMarketData(const string &sym) {
    FixMessage mdr(FIX::MsgType_MarketDataRequest);
    mdr.setField(FIX::FIELD::MDReqID, "MDR" + to_string(fixSeqNum_));
    mdr.setField(FIX::FIELD::SubscriptionRequestType, std::string(1, FIX::SubscriptionRequestType_SNAPSHOT));
    mdr.setField(FIX::FIELD::MarketDepth, "0");
    mdr.setField(FIX::FIELD::NoMDEntryTypes, "2");
    mdr.setField(FIX::FIELD::MDEntryType, "0"); // Bid
    mdr.setField(FIX::FIELD::MDEntryType, "1"); // Offer
    mdr.setField(FIX::FIELD::NoRelatedSym, "1");
    mdr.setField(FIX::FIELD::Symbol, sym);
    sendFixMessage(mdr);
  }

  void sendNewOrder() {
    string sym = sym_in->get_value();
    string qty = qty_in->get_value();
    string px = px_in->get_value();
    string side = side_sel->get_value();
    if (side.empty())
      side = std::string(1, FIX::Side_BUY); // fallback

    if (sym.empty() || qty.empty() || px.empty())
      return;

    FixMessage nos(FIX::MsgType_NewOrderSingle);
    string clOrdId = generateOrderId();
    nos.setField(FIX::FIELD::ClOrdID, clOrdId);
    nos.setField(FIX::FIELD::HandlInst, "1");
    nos.setField(FIX::FIELD::Symbol, sym);
    nos.setField(FIX::FIELD::Side, side);
    nos.setField(FIX::FIELD::TransactTime, getTimestamp());
    nos.setField(FIX::FIELD::OrderQty, qty);
    nos.setField(FIX::FIELD::OrdType, std::string(1, FIX::OrdType_LIMIT));
    nos.setField(FIX::FIELD::Price, px);

    OrderData o;
    o.id = clOrdId;
    o.origId = clOrdId;
    o.symbol = sym;
    o.side = (side == std::string(1, FIX::Side_BUY)) ? "Buy" : "Sell";
    o.type = "Limit";
    o.price = stod(px);
    o.qty = stol(qty);
    o.leaves = o.qty;
    o.status = "New";
    orders[clOrdId] = o;

    sendFixMessage(nos);
    updateOrderRow(clOrdId);
  }

  void replaceSelected() {
    if (selectedOrderId.empty())
      return;
    if (orders.find(selectedOrderId) == orders.end())
      return;

    OrderData &o = orders[selectedOrderId];
    // Only replace active orders
    if (o.status != "New" && o.status != "Partial" && o.status != "Replaced")
      return;

    string px = repPxIn->get_value();
    string qty = repQtyIn->get_value();
    if (px.empty() || qty.empty())
      return;

    FixMessage rep(FIX::MsgType_OrderCancelReplaceRequest);
    string newClOrdId = generateOrderId();
    rep.setField(FIX::FIELD::OrigClOrdID, o.id);
    rep.setField(FIX::FIELD::ClOrdID, newClOrdId);
    rep.setField(FIX::FIELD::Symbol, o.symbol);
    rep.setField(FIX::FIELD::Side, (o.side == "Buy") ? std::string(1, FIX::Side_BUY) : std::string(1, FIX::Side_SELL));
    rep.setField(FIX::FIELD::TransactTime, getTimestamp());
    rep.setField(FIX::FIELD::OrderQty, qty);
    rep.setField(FIX::FIELD::OrdType, std::string(1, FIX::OrdType_LIMIT));
    rep.setField(FIX::FIELD::Price, px);

    // Track the new order locally anticipating success
    OrderData no = o;
    no.origId = o.id;
    no.id = newClOrdId;
    no.price = stod(px);
    no.qty = stol(qty);
    orders[newClOrdId] = no;

    sendFixMessage(rep);
  }

  void cancelSelected() {
    if (selectedOrderId.empty())
      return;
    if (orders.find(selectedOrderId) == orders.end())
      return;
    OrderData &o = orders[selectedOrderId];
    if (o.status != "New" && o.status != "Partial" && o.status != "Replaced")
      return;
    doCancel(selectedOrderId);
  }

  void doCancel(const string &clOrdId) {
    if (orders.find(clOrdId) == orders.end())
      return;
    OrderData &o = orders[clOrdId];
    // Only cancel active orders
    if (o.status != "New" && o.status != "Partial" && o.status != "Replaced")
      return;

    FixMessage cxl(FIX::MsgType_OrderCancelRequest);
    string newClOrdId = generateOrderId();
    cxl.setField(FIX::FIELD::OrigClOrdID, o.id);
    cxl.setField(FIX::FIELD::ClOrdID, newClOrdId);
    cxl.setField(FIX::FIELD::Symbol, o.symbol);
    cxl.setField(FIX::FIELD::Side, (o.side == "Buy") ? std::string(1, FIX::Side_BUY) : std::string(1, FIX::Side_SELL));
    cxl.setField(FIX::FIELD::TransactTime, getTimestamp());
    sendFixMessage(cxl);
  }

  void cancelOrdersBySide(const string &sideFilter) {
    for (const auto &pair : orders) {
      const OrderData &o = pair.second;
      if (o.status == "New" || o.status == "Partial" ||
          o.status == "Replaced") {
        if (sideFilter.empty() || o.side == sideFilter) {
          doCancel(o.id);
        }
      }
    }
  }

  void updateMarketRow(const string &sym) {
    if (md_rows.find(sym) == md_rows.end()) {
      auto tr = std::make_shared<Element>(ctx_, ctx_->strings.tr);
      ctx_->register_element(tr);
      tr->add_event_listener(
          ctx_->strings.click, [this, tr, sym](const Event &) {
            if (selMdRow)
              selMdRow->set_attribute(ctx_->strings.style, "");
            tr->set_attribute(ctx_->strings.style,
                              "background-color: #d1ecf1; cursor: pointer;");
            selMdRow = tr;
            selectedMdSym = sym;
          });
      tr->set_attribute(ctx_->strings.style, "cursor: pointer;");
      vector<shared_ptr<Element>> cells;
      for (int i = 0; i < 8; i++) {
        auto td = std::make_shared<Element>(ctx_, ctx_->strings.td);
        ctx_->register_element(td);
        td->set_attribute(ctx_->strings.style,
                          "border: 1px solid #ccc; padding: 5px;");
        tr->append_child(td);
        cells.push_back(td);
      }
      md_table->append_child(tr);
      md_rows[sym] = cells;
      md_trs[sym] = tr;
    }

    auto &cells = md_rows[sym];
    const auto &m = markets[sym];
    vector<string> vals = {m.symbol,
                           to_string(m.bidSz),
                           to_string(m.bidPx),
                           to_string(m.askPx),
                           to_string(m.askSz),
                           to_string(m.lastPx),
                           to_string(m.lastSz),
                           to_string(m.volume)};
    for (int i = 0; i < 8; i++) {
      cells[i]->set_text_content(ctx_->register_string(vals[i]));
    }
  }

  void updateOrderRow(const string &id) {
    if (ord_rows.find(id) == ord_rows.end()) {
      auto tr = std::make_shared<Element>(ctx_, ctx_->strings.tr);
      ctx_->register_element(tr);
      tr->add_event_listener(
          ctx_->strings.click, [this, tr, id](const Event &) {
            if (selOrdRow)
              selOrdRow->set_attribute(ctx_->strings.style, "");
            tr->set_attribute(ctx_->strings.style,
                              "background-color: #d1ecf1; cursor: pointer;");
            selOrdRow = tr;
            selectedOrderId = id;
            selOrdIdDisp->set_text_content(ctx_->register_string(id));
            if (orders.find(id) != orders.end()) {
              repQtyIn->set_value(to_string(orders[id].qty));
              repPxIn->set_value(to_string(orders[id].price));
            }
          });
      tr->set_attribute(ctx_->strings.style, "cursor: pointer;");
      vector<shared_ptr<Element>> cells;
      for (int i = 0; i < 8; i++) {
        auto td = std::make_shared<Element>(ctx_, ctx_->strings.td);
        ctx_->register_element(td);
        td->set_attribute(ctx_->strings.style,
                          "border: 1px solid #ccc; padding: 5px;");
        tr->append_child(td);
        cells.push_back(td);
      }
      ord_table->append_child(tr);
      ord_rows[id] = cells;
      ord_trs[id] = tr;
    }

    auto &cells = ord_rows[id];
    const auto &o = orders[id];
    vector<string> vals = {o.id,
                           o.symbol,
                           o.side,
                           to_string(o.qty),
                           to_string(o.price),
                           to_string(o.leaves),
                           to_string(o.avgPx),
                           o.status};
    for (int i = 0; i < 8; i++) {
      cells[i]->set_text_content(ctx_->register_string(vals[i]));
    }
  }

  void addExecutionRow(const string &execType, const FixMessage &msg) {
    ExecutionData e;
    e.id = msg.getField(FIX::FIELD::ClOrdID);
    e.execId = msg.getField(FIX::FIELD::ExecID);
    e.symbol = msg.getField(FIX::FIELD::Symbol);
    e.side = (msg.getField(FIX::FIELD::Side) == std::string(1, FIX::Side_BUY)) ? "Buy" : "Sell";

    auto execTransType = msg.getField(FIX::FIELD::ExecTransType);

    if (execType == std::string(1, FIX::ExecType_TRADE) || execTransType == std::string(1, FIX::ExecTransType_NEW))
      e.execType = "TRADE";
    else if (execType == std::string(1, FIX::ExecType_TRADE_CORRECT) || execTransType == std::string(1, FIX::ExecTransType_CANCEL))
      e.execType = "BUST";
    else if (execType == std::string(1, FIX::ExecType_TRADE_CANCEL) || execTransType == std::string(1, FIX::ExecTransType_CORRECT))
      e.execType = "CORRECTED";

    if (!msg.getField(FIX::FIELD::LastPx).empty())
      e.lastPx = stod(msg.getField(FIX::FIELD::LastPx));
    if (!msg.getField(FIX::FIELD::LastShares).empty())
      e.lastSz = stol(msg.getField(FIX::FIELD::LastShares));
    if (!msg.getField(FIX::FIELD::CumQty).empty())
      e.cumQty = stol(msg.getField(FIX::FIELD::CumQty));

    auto tr = std::make_shared<Element>(ctx_, ctx_->strings.tr);
    ctx_->register_element(tr);
    vector<string> vals = {e.execId,
                           e.id,
                           e.symbol,
                           e.side,
                           to_string(e.lastPx),
                           to_string(e.lastSz),
                           to_string(e.cumQty),
                           e.execType};
    for (const auto &v : vals) {
      auto td = std::make_shared<Element>(ctx_, ctx_->strings.td);
      ctx_->register_element(td);
      td->set_attribute(ctx_->strings.style,
                        "border: 1px solid #ccc; padding: 5px;");
      td->set_text_content(ctx_->register_string(v));
      tr->append_child(td);
    }
    trd_table->append_child(tr);
  }

  void addRejectRow(const FixMessage &msg) {
    string stat = msg.getField(FIX::FIELD::OrdStatus);
    string cxlRejRes = msg.getField(FIX::FIELD::CxlRejResponseTo);
    string type = (cxlRejRes == std::string(1, FIX::CxlRejResponseTo_ORDER_CANCEL_REQUEST)) ? "CancelRequest" : "CancelReplaceRequest";
    if (msg.msgType == FIX::MsgType_ExecutionReport)
      type = "NewOrderSingle"; // ExecutionReport reject

    auto tr = std::make_shared<Element>(ctx_, ctx_->strings.tr);
    ctx_->register_element(tr);
    vector<string> vals = {msg.getField(FIX::FIELD::ClOrdID), msg.getField(FIX::FIELD::OrigClOrdID), type, "Rejected",
                           msg.getField(FIX::FIELD::Text)};
    for (const auto &v : vals) {
      auto td = std::make_shared<Element>(ctx_, ctx_->strings.td);
      ctx_->register_element(td);
      td->set_attribute(ctx_->strings.style,
                        "border: 1px solid #ccc; padding: 5px;");
      td->set_text_content(ctx_->register_string(v));
      tr->append_child(td);
    }
    rej_table->append_child(tr);
  }

  void handleMarketData(const FixMessage &msg) {
    string sym = msg.getField(FIX::FIELD::Symbol);
    if (sym.empty())
      return;
    MarketData &m = markets[sym];
    m.symbol = sym;

    m.bidPx = 0;
    m.bidSz = 0;
    m.askPx = 0;
    m.askSz = 0;

    string currentType;
    for (const auto &f : msg.fields) {
      if (f.first == FIX::FIELD::MDEntryType)
        currentType = f.second;
      else if (f.first == FIX::FIELD::MDEntryPx) {
        if (currentType == std::string(1, FIX::MDEntryType_BID))
          m.bidPx = stod(f.second);
        else if (currentType == std::string(1, FIX::MDEntryType_OFFER))
          m.askPx = stod(f.second);
        else if (currentType == std::string(1, FIX::MDEntryType_TRADE))
          m.lastPx = stod(f.second);
      } else if (f.first == FIX::FIELD::MDEntrySize) {
        if (currentType == std::string(1, FIX::MDEntryType_BID))
          m.bidSz = stol(f.second);
        else if (currentType == std::string(1, FIX::MDEntryType_OFFER))
          m.askSz = stol(f.second);
        else if (currentType == std::string(1, FIX::MDEntryType_TRADE))
          m.lastSz = stol(f.second);
        else if (currentType == std::string(1, FIX::MDEntryType_TRADE_VOLUME))
          m.volume = stol(f.second);
      }
    }
    updateMarketRow(sym);
  }

  void handleExecutionReport(const FixMessage &msg) {
    string id = msg.getField(FIX::FIELD::ClOrdID);
    string origId = msg.getField(FIX::FIELD::OrigClOrdID);
    string status = msg.getField(FIX::FIELD::OrdStatus);

    if (status == std::string(1, FIX::OrdStatus_REJECTED)) {
      addRejectRow(msg);
    }

    if (orders.find(id) != orders.end()) {
      OrderData &o = orders[id];
      if (!msg.getField(FIX::FIELD::LeavesQty).empty())
        o.leaves = stol(msg.getField(FIX::FIELD::LeavesQty));
      if (!msg.getField(FIX::FIELD::CumQty).empty())
        o.cumQty = stol(msg.getField(FIX::FIELD::CumQty));
      if (!msg.getField(FIX::FIELD::AvgPx).empty())
        o.avgPx = stod(msg.getField(FIX::FIELD::AvgPx));

      if (status == std::string(1, FIX::OrdStatus_NEW))
        o.status = "New";
      else if (status == std::string(1, FIX::OrdStatus_PARTIALLY_FILLED))
        o.status = "Partial";
      else if (status == std::string(1, FIX::OrdStatus_FILLED))
        o.status = "Filled";
      else if (status == std::string(1, FIX::OrdStatus_CANCELED))
        o.status = "Canceled";
      else if (status == std::string(1, FIX::OrdStatus_REPLACED))
        o.status = "Replaced";
      else if (status == std::string(1, FIX::OrdStatus_REJECTED))
        o.status = "Rejected";
      updateOrderRow(id);

      if (status == std::string(1, FIX::OrdStatus_REPLACED)) {
        if (!origId.empty() && orders.find(origId) != orders.end()) {
          // Unhandled replace logic fallback
          OrderData &orig = orders[origId];
          orig.id = origId;
          orig.status = "Canceled & Replaced";
          orders[origId] = orig;
          updateOrderRow(origId);
          if (ord_trs.find(origId) != ord_trs.end()) {
            ord_trs[origId]->set_attribute(ctx_->strings.style,
                                           "display: none;");
          }
        }
      }
    } else if (!origId.empty() && orders.find(origId) != orders.end()) {
      // Unhandled replace logic fallback
      OrderData &orig = orders[origId];
      orig.id = id;
      if (status == std::string(1, FIX::OrdStatus_REPLACED))
        orig.status = "Replaced";
      else if (status == std::string(1, FIX::OrdStatus_CANCELED))
        orig.status = "Canceled";
      orders[id] = orig;
      updateOrderRow(id);
      if (ord_trs.find(origId) != ord_trs.end()) {
        ord_trs[origId]->set_attribute(ctx_->strings.style, "display: none;");
      }
    }

    auto execType = msg.getField(FIX::FIELD::ExecType);

    if (execType == std::string(1, FIX::ExecType_TRADE) || execType == std::string(1, FIX::ExecType_PARTIAL_FILL) || execType == std::string(1, FIX::ExecType_FILL) ||
        execType == std::string(1, FIX::ExecType_TRADE_CORRECT) || execType == std::string(1, FIX::ExecType_TRADE_CANCEL)) {
      addExecutionRow(execType, msg);
    }
  }

  void handleCancelReject(const FixMessage &msg) {
    addRejectRow(msg);
    // Clean up pending replaced orders
    string id = msg.getField(FIX::FIELD::ClOrdID);
    if (orders.find(id) != orders.end() && orders[id].status == "New") {
      // It was a pending replace/cancel that failed. We can just mark it
      // rejected locally
      orders[id].status = "Rejected";
      updateOrderRow(id);
    }
  }

  static EM_BOOL onOpen(int eventType, const EmscriptenWebSocketOpenEvent *ev,
                        void *userData) {
    BanzaiExchangeApp *app = (BanzaiExchangeApp *)userData;
    app->sessionState_ = CONNECTED;
    app->updateStatusBar();

    FixMessage logon(FIX::MsgType_Logon);
    logon.setField(FIX::FIELD::EncryptMethod, "0");
    logon.setField(FIX::FIELD::HeartBtInt, "30");
    app->logonSentTime_ = emscripten_get_now();
    app->sendFixMessage(logon);
    return EM_TRUE;
  }

  static EM_BOOL onClose(int eventType, const EmscriptenWebSocketCloseEvent *ev,
                         void *userData) {
    BanzaiExchangeApp *app = (BanzaiExchangeApp *)userData;
    app->ws_ = 0;
    app->sessionState_ = DISCONNECTED;
    app->lastConnectAttemptTime_ = 0.0;
    app->updateStatusBar();
    return EM_TRUE;
  }

  static EM_BOOL onError(int eventType, const EmscriptenWebSocketErrorEvent *ev,
                         void *userData) {
    BanzaiExchangeApp *app = (BanzaiExchangeApp *)userData;
    app->ws_ = 0;
    app->sessionState_ = DISCONNECTED;
    app->lastConnectAttemptTime_ = 0.0;
    app->updateStatusBar();
    return EM_TRUE;
  }

  static EM_BOOL onMessage(int eventType,
                           const EmscriptenWebSocketMessageEvent *ev,
                           void *userData) {
    BanzaiExchangeApp *app = (BanzaiExchangeApp *)userData;
    std::string raw((const char *)ev->data, ev->numBytes);
    FixMessage msg = FixMessage::parse(raw);

    string seqStr = msg.getField(FIX::FIELD::MsgSeqNum);
    if (!seqStr.empty()) {
      app->targetSeqNum_ = stoi(seqStr);
      app->updateStatusBar();
    }

    if (msg.msgType == FIX::MsgType_Logon) {
      app->sessionState_ = LOGGED_ON;
      app->updateStatusBar();

      // Setup initial symbols
      const char *syms[] = {"LNUX", "YHOO", "CSCO"};
      for (int i = 0; i < 3; i++) {
        app->requestMarketData(syms[i]);
      }

      // Setup initial orders
      for (int i = 0; i < 3; i++) {
        FixMessage nos(FIX::MsgType_NewOrderSingle);
        string clOrdId = app->generateOrderId();
        nos.setField(FIX::FIELD::ClOrdID, clOrdId);
        nos.setField(FIX::FIELD::HandlInst, "1");
        nos.setField(FIX::FIELD::Symbol, syms[i]);
        nos.setField(FIX::FIELD::Side, std::string(1, FIX::Side_BUY));
        nos.setField(FIX::FIELD::TransactTime, getTimestamp());
        nos.setField(FIX::FIELD::OrderQty, "100");
        nos.setField(FIX::FIELD::OrdType, std::string(1, FIX::OrdType_LIMIT));
        nos.setField(FIX::FIELD::Price, "10.5");
        app->sendFixMessage(nos);

        OrderData o;
        o.id = clOrdId;
        o.origId = clOrdId;
        o.symbol = syms[i];
        o.side = "Buy";
        o.type = "Limit";
        o.price = 10.5;
        o.qty = 100;
        o.leaves = 100;
        o.status = "New";
        app->orders[clOrdId] = o;
        app->updateOrderRow(clOrdId);
      }
    } else if (msg.msgType == FIX::MsgType_MarketDataSnapshotFullRefresh) {
      app->handleMarketData(msg);
    } else if (msg.msgType == FIX::MsgType_ExecutionReport) {
      app->handleExecutionReport(msg);
    } else if (msg.msgType == FIX::MsgType_OrderCancelReject) {
      app->handleCancelReject(msg);
    } else if (msg.msgType == FIX::MsgType_TestRequest) {
      string testReqId = msg.getField(FIX::FIELD::TestReqID);
      app->sendHeartbeat(testReqId);
    } else if (msg.msgType == FIX::MsgType_BusinessMessageReject) {
      string text = msg.getField(FIX::FIELD::Text);
      if (text == "stock is not known") {
        app->showStockNotKnownAlert();
      }
      app->handleCancelReject(msg);
    }

    return EM_TRUE;
  }

  void updateStatusBar() {
    if (!status_bar) return;
    
    std::string lightStyle = "width: 12px; height: 12px; border-radius: 50%; display: inline-block; margin-right: 8px; vertical-align: middle;";
    std::string textLabel = "";
    
    if (sessionState_ == DISCONNECTED) {
      lightStyle += " background-color: #ff3b30; box-shadow: 0 0 8px #ff3b30;";
      textLabel = "DISCONNECTED | FIX.4.2:BANZAI->ORDERMATCH";
    } else if (sessionState_ == CONNECTED) {
      lightStyle += " background-color: #ffcc00; box-shadow: 0 0 8px #ffcc00;";
      textLabel = "CONNECTED (LOGGING ON) | FIX.4.2:BANZAI->ORDERMATCH";
    } else if (sessionState_ == LOGGED_ON) {
      lightStyle += " background-color: #34c759; box-shadow: 0 0 8px #34c759;";
      textLabel = "LOGGED ON | FIX.4.2:BANZAI->ORDERMATCH";
    }
    
    status_light->set_attribute(ctx_->strings.style, lightStyle);
    status_text->set_text_content(ctx_->register_string(textLabel));
    
    std::string seqs = std::to_string(senderSeqNum_) + ":" + std::to_string(targetSeqNum_);
    seq_num_text->set_text_content(ctx_->register_string(seqs));
  }

  void connectWebSocket() {
    if (ws_) return;
    
    sessionState_ = DISCONNECTED;
    updateStatusBar();
    
    EmscriptenWebSocketCreateAttributes ws_attrs = {"ws://localhost:3000/fix",
                                                    NULL, EM_TRUE};
    ws_ = emscripten_websocket_new(&ws_attrs);
    if (ws_ > 0) {
      emscripten_websocket_set_onopen_callback(ws_, this, onOpen);
      emscripten_websocket_set_onmessage_callback(ws_, this, onMessage);
      emscripten_websocket_set_onclose_callback(ws_, this, onClose);
      emscripten_websocket_set_onerror_callback(ws_, this, onError);
    } else {
      ws_ = 0;
    }
    lastConnectAttemptTime_ = emscripten_get_now();
  }

private:
  std::shared_ptr<HTMLDivElement> status_bar;
  std::shared_ptr<Element> status_light;
  std::shared_ptr<Element> status_text;
  std::shared_ptr<Element> seq_num_text;

  enum SessionState {
    DISCONNECTED,
    CONNECTED,
    LOGGED_ON
  };
  int sessionState_ = DISCONNECTED;
  int senderSeqNum_ = 0;
  int targetSeqNum_ = 0;
  double lastConnectAttemptTime_ = 0.0;
  double logonSentTime_ = 0.0;

  Context *ctx_ = nullptr;
  std::shared_ptr<HTMLDivElement> main_div;
  std::shared_ptr<HTMLDivElement> error_alert;
  double lastSentTime_ = 0.0;
  double alertStartTime_ = 0.0;
  bool alertActive_ = false;
  std::shared_ptr<HTMLDivElement> tabOrders;
  std::shared_ptr<HTMLDivElement> tabTrades;
  std::shared_ptr<HTMLDivElement> tabRejects;

  std::shared_ptr<Element> md_table;
  std::shared_ptr<Element> ord_table;
  std::shared_ptr<Element> trd_table;
  std::shared_ptr<Element> rej_table;

  std::shared_ptr<HTMLInputElement> sym_in;
  std::shared_ptr<HTMLInputElement> qty_in;
  std::shared_ptr<HTMLInputElement> px_in;
  std::shared_ptr<HTMLSelectElement> side_sel;

  std::shared_ptr<Element> selMdRow;
  string selectedMdSym;

  std::shared_ptr<Element> selOrdRow;
  string selectedOrderId;
  std::shared_ptr<HTMLDivElement> selOrdIdDisp;
  std::shared_ptr<HTMLInputElement> repQtyIn;
  std::shared_ptr<HTMLInputElement> repPxIn;

  EMSCRIPTEN_WEBSOCKET_T ws_ = 0;
  int fixSeqNum_ = 1;
  int orderIdCounter_ = 1;

  std::map<string, MarketData> markets;
  std::map<string, OrderData> orders;

  std::map<string, vector<shared_ptr<Element>>> md_rows;
  std::map<string, shared_ptr<Element>> md_trs;

  std::map<string, vector<shared_ptr<Element>>> ord_rows;
  std::map<string, shared_ptr<Element>> ord_trs;
};

BanzaiExchangeApp *g_app_instance = nullptr;