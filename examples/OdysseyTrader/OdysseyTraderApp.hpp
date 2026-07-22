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
#include "NetzWirbel/App.hpp"
#include "NetzWirbel/DOM/Grid.hpp"
#include "NetzWirbel/DOM/Window.hpp"
#include "NetzWirbel/DOM/Button.hpp"
#include "NetzWirbel/DOM/Spinner.hpp"
#include "NetzWirbel/DOM/TextBox.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include "Protocol.h"
#include <algorithm>
#include <arpa/inet.h>
#include <emscripten.h>
#include <emscripten/websocket.h>
#include "NetzWirbel/DOM/Button.hpp"
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include "OverAlignedAllocator.hpp"

using namespace NetzWirbel;


struct OdysseyMarketData {
  std::string symbol;
  uint32_t bidSz;
  double bidPx;
  uint32_t askSz;
  double askPx;
  uint32_t lastSz;
  double lastPx;
  uint32_t volume;
  std::shared_ptr<GridRow<OdysseyMarketData>> row_ptr;
  bool is_invalid = false;
};

struct OdysseyOrderData {
  std::string id;
  std::string symbol;
  std::string side;
  std::string type;
  std::string status;
  double price;
  uint32_t qty;
  uint32_t leaves;
  uint32_t cumQty;
  double avgPx;
};

struct OdysseyExecutionData {
  std::string id;
  std::string symbol;
  std::string side;
  std::string execType;
  uint32_t lastSz;
  double lastPx;
  uint32_t cumQty;
  double avgPx;
};

struct OdysseyRejectData {
  std::string id;
  std::string refMsgType;
  std::string status;
  std::string text;
};


class OdysseyTraderApp;
extern OdysseyTraderApp *g_odyssey_app_instance;

class OdysseyTraderApp : public App {
private:
  std::string generateOrderId() {
      unsigned long a = lrand48() & 0xFFFF;
      unsigned long b = lrand48() & 0xFFFF;
      unsigned long c = lrand48() & 0xFFFF;
      char buf[64];
      snprintf(buf, sizeof(buf), "%04lx-%04lx-%04lx", a, b, c);
      return std::string(buf);
  }

  // Declaring all private member variables at the top of the private section
  Context *ctx_ = nullptr;
  EMSCRIPTEN_WEBSOCKET_T ws_ = 0;
  std::vector<uint8_t, OverAlignedAllocator<uint8_t, 16>> socketRecvBuf_;
  int connected_state =
      0; // 0 = DISCONNECTED, 1 = CONNECTED (Amber), 2 = LOGGED_ON (Green)
  int last_connected_state = -1;
  double last_connect_attempt_ = 0.0;
  double last_logon_sent_time_ = 0.0;
  double last_heartbeat_time_ = 0.0;
  std::string username_;
  std::string session_name_ = "ODYSSEY_SESSION";

  // Sequence tracking
  uint32_t sender_seq_num_ = 0;
  uint32_t target_seq_num_ = 0;

  // UI elements
  std::shared_ptr<HTMLDivElement> main_div;
  std::shared_ptr<HTMLDivElement> status_bar;
  std::shared_ptr<Element> status_light;
  std::shared_ptr<Element> status_text;
  std::shared_ptr<Element> seq_num_text;
  std::shared_ptr<HTMLDivElement> taskbar_;
  std::shared_ptr<HTMLDivElement> logon_prompt;
  std::shared_ptr<HTMLInputElement> username_input;
  bool status_bar_visible_ = false;

  // Dialogue prompts
  std::shared_ptr<HTMLDivElement> order_dialog;
  std::shared_ptr<Element> order_dialog_title;
  std::shared_ptr<HTMLInputElement> order_symbol_input;
  std::shared_ptr<HTMLInputElement> order_qty_input;
  std::shared_ptr<HTMLInputElement> order_price_input;
  std::shared_ptr<Button> order_dialog_submit_btn;
  std::string order_dialog_side; // "Buy" or "Sell"
  std::string replace_order_id;  // Non-empty if modifying
  bool order_dialog_visible_ = false;

  // Window States
  std::shared_ptr<Window> md_win_, orders_win_, exec_win_, rej_win_, pref_win_;
  std::shared_ptr<Grid<OdysseyMarketData>> md_grid_;
  std::shared_ptr<Grid<OdysseyOrderData>> order_grid_;
  std::shared_ptr<NetzWirbel::HTMLDivElement> order_row_;
  std::shared_ptr<Grid<OdysseyExecutionData>> exec_grid_;
  std::shared_ptr<Grid<OdysseyRejectData>> reject_grid_;
  
  // Preferences
  int pref_default_qty_ = 100;
  double pref_max_aggressive_ = 5.0; // 5%
  double pref_max_notional_ = 1000000.0;
  std::shared_ptr<HTMLInputElement> pref_qty_input_, pref_agg_input_, pref_not_input_;

  
  int top_z_index = 100;

  // Local lists
  std::vector<OdysseyMarketData> market_data_;
  using OrdersList = std::list<OdysseyOrderData>;
  OrdersList orders_;
  std::unordered_map<std::string, OrdersList::iterator> orders_map_;
  std::vector<OdysseyExecutionData> executions_;
  std::vector<OdysseyRejectData> rejections_;
  std::set<std::string> subscribed_symbols_;
  std::string selected_order_id_;

  // DOM containers in windows
  std::shared_ptr<Element> md_table_body;
  std::shared_ptr<Element> orders_table_body;
  std::shared_ptr<Element> execs_table_body;
  std::shared_ptr<Element> rejects_table_body;

  // Grid content elements to dock/undock
  std::shared_ptr<HTMLDivElement> orders_grid_cont;
  std::shared_ptr<HTMLDivElement> execs_grid_cont;
  std::shared_ptr<HTMLDivElement> rejects_grid_cont;

  // Tab buttons in main orders window
  std::shared_ptr<HTMLDivElement> tab_bar;
  std::shared_ptr<Element> tab_btn_orders;
  std::shared_ptr<Element> tab_btn_execs;
  std::shared_ptr<Element> tab_btn_rejects;
  std::string active_orders_tab =
      "orders"; // "orders", "executions", "rejections"

  // Private helper methods
  void save_layout() {
    if (username_.empty()) return;
    std::stringstream ss;
    ss << "{";
    ss << "\"pref_default_qty\": " << pref_default_qty_ << ", ";
    ss << "\"pref_max_aggressive\": " << pref_max_aggressive_ << ", ";
    ss << "\"pref_max_notional\": " << pref_max_notional_;
    
    auto append_win = [&](const std::string& key, std::shared_ptr<Window> w) {
        if (!w) return;
        ss << ", \"" << key << "\": {\"x\": " << w->get_x() 
           << ", \"y\": " << w->get_y()
           << ", \"w\": " << w->get_width()
           << ", \"h\": " << w->get_height()
           << ", \"maximized\": " << (w->is_maximized() ? "true" : "false")
           << ", \"minimized\": " << (w->is_minimized() ? "true" : "false") << "}";
    };
    append_win("market_data", md_win_);
    append_win("orders", orders_win_);
    append_win("executions", exec_win_);
    append_win("rejections", rej_win_);
    append_win("preferences", pref_win_);
    
    ss << ", \"market_data_symbols\": [";
    bool first = true;
    for (const auto& m : market_data_) {
        if (!m.symbol.empty()) {
            if (!first) ss << ", ";
            ss << "\"" << m.symbol << "\"";
            first = false;
        }
    }
    ss << "]";
    
    ss << "}";

    std::string layout_msg = "SAVE_LAYOUT|" + username_ + "|" + ss.str();
    emscripten_websocket_send_utf8_text(ws_, layout_msg.c_str());
}

  void handle_layout_response(const std::string &layout_json) {
    pref_default_qty_ = extract_json_int(layout_json, 0, "pref_default_qty", pref_default_qty_);
    
    size_t agg_pos = layout_json.find("\"pref_max_aggressive\"");
    if (agg_pos != std::string::npos) {
        size_t colon_pos = layout_json.find(":", agg_pos);
        size_t comma_pos = layout_json.find_first_of(",}", colon_pos);
        if (colon_pos != std::string::npos && comma_pos != std::string::npos) {
            std::string val = layout_json.substr(colon_pos + 1, comma_pos - colon_pos - 1);
            try { pref_max_aggressive_ = std::stod(val); } catch (...) {}
        }
    }
    
    size_t not_pos = layout_json.find("\"pref_max_notional\"");
    if (not_pos != std::string::npos) {
        size_t c = layout_json.find(":", not_pos);
        if (c != std::string::npos) {
          size_t n = layout_json.find_first_of(",}", c);
          std::string val = layout_json.substr(c+1, n - c - 1);
          try { pref_max_notional_ = std::stod(val); } catch (...) {}
        }
    }
    
    if (pref_qty_input_) {
        pref_qty_input_->set_value(std::to_string(pref_default_qty_));
    }
    if (pref_agg_input_) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << pref_max_aggressive_;
        pref_agg_input_->set_value(ss.str());
    }
    if (pref_not_input_) {
        std::stringstream ss;
        ss << (long long)pref_max_notional_;
        pref_not_input_->set_value(ss.str());
    }
    
    auto extract_win = [&](const std::string& key, std::shared_ptr<Window> w) {
        if (!w) return;
        size_t pos = layout_json.find("\"" + key + "\"");
        if (pos != std::string::npos) {
            int x = extract_json_int(layout_json, pos, "x", w->get_x());
            int y = extract_json_int(layout_json, pos, "y", w->get_y());
            int width = extract_json_int(layout_json, pos, "w", w->get_width());
            int height = extract_json_int(layout_json, pos, "h", w->get_height());
            w->set_bounds(x, y, width, height);
            
            size_t max_pos = layout_json.find("\"maximized\"", pos);
            if (max_pos != std::string::npos) {
                size_t colon_pos = layout_json.find(":", max_pos);
                if (colon_pos != std::string::npos) {
                    if (layout_json.find("true", colon_pos) != std::string::npos && layout_json.find("true", colon_pos) < layout_json.find_first_of(",}", colon_pos)) {
                        w->set_maximized(true);
                    } else {
                        w->set_maximized(false);
                    }
                }
            }
            
            size_t min_pos = layout_json.find("\"minimized\"", pos);
            if (min_pos != std::string::npos) {
                size_t colon_pos = layout_json.find(":", min_pos);
                if (colon_pos != std::string::npos) {
                    if (layout_json.find("true", colon_pos) != std::string::npos && layout_json.find("true", colon_pos) < layout_json.find_first_of(",}", colon_pos)) {
                        w->set_minimized(true);
                    } else {
                        w->set_minimized(false);
                    }
                }
            }
        }
    };
    extract_win("market_data", md_win_);
    extract_win("orders", orders_win_);
    extract_win("executions", exec_win_);
    extract_win("rejections", rej_win_);
    extract_win("preferences", pref_win_);
    
    size_t syms_pos = layout_json.find("\"market_data_symbols\"");
    if (syms_pos != std::string::npos) {
        size_t bracket_start = layout_json.find("[", syms_pos);
        size_t bracket_end = layout_json.find("]", bracket_start);
        if (bracket_start != std::string::npos && bracket_end != std::string::npos) {
            std::string arr = layout_json.substr(bracket_start + 1, bracket_end - bracket_start - 1);
            
            for (const auto& m : market_data_) {
                if (!m.symbol.empty()) unsubscribe_symbol(m.symbol);
            }
            market_data_.clear();
            
            size_t i = 0;
            while (i < arr.size()) {
                size_t q1 = arr.find("\"", i);
                if (q1 == std::string::npos) break;
                size_t q2 = arr.find("\"", q1 + 1);
                if (q2 == std::string::npos) break;
                std::string sym = arr.substr(q1 + 1, q2 - q1 - 1);
                
                OdysseyMarketData md;
                md.symbol = sym;
                md.bidPx = 0; md.bidSz = 0; md.askPx = 0; md.askSz = 0;
                md.lastPx = 0; md.lastSz = 0; md.volume = 0; md.is_invalid = false;
                market_data_.push_back(md);
                
                subscribe_symbol(sym);
                
                i = q2 + 1;
            }
            
            market_data_.push_back(OdysseyMarketData{"", 0, 0, 0, 0, 0, 0, 0, nullptr, false});
            rebuild_market_data_grid();
        }
    }
}

  int extract_json_int(const std::string &json, size_t offset,
                       const std::string &key, int default_val) {
    size_t key_pos = json.find("\"" + key + "\"", offset);
    if (key_pos == std::string::npos)
      return default_val;
    size_t colon_pos = json.find(":", key_pos);
    if (colon_pos == std::string::npos)
      return default_val;

    size_t end_pos = json.find_first_of(",}", colon_pos);
    std::string val_str = json.substr(colon_pos + 1, end_pos - colon_pos - 1);
    try {
      return std::stoi(val_str);
    } catch (...) {
      return default_val;
    }
  }

  bool extract_json_bool(const std::string &json, size_t offset,
                         const std::string &key, bool default_val) {
    size_t key_pos = json.find("\"" + key + "\"", offset);
    if (key_pos == std::string::npos)
      return default_val;
    size_t colon_pos = json.find(":", key_pos);
    if (colon_pos == std::string::npos)
      return default_val;

    size_t end_pos = json.find_first_of(",}", colon_pos);
    std::string val_str = json.substr(colon_pos + 1, end_pos - colon_pos - 1);
    val_str.erase(std::remove_if(val_str.begin(), val_str.end(), ::isspace),
                  val_str.end());
    if (val_str == "true")
      return true;
    if (val_str == "false")
      return false;
    return default_val;
  }

  void show_alert(const std::string& msg) {
    MAIN_THREAD_EM_ASM({
        alert(UTF8ToString($0));
    }, msg.c_str());
  }

  void init_windows() {
    Window::attach_manager(main_div, ctx_);
    
    md_win_ = std::make_shared<Window>(ctx_, "market_data", "Market Data Grid", 50, 50, 600, 350);
    orders_win_ = std::make_shared<Window>(ctx_, "orders", "Orders & Workspaces", 680, 50, 700, 350);
    exec_win_ = std::make_shared<Window>(ctx_, "executions", "Standalone Executions", 50, 420, 600, 300);
    rej_win_ = std::make_shared<Window>(ctx_, "rejections", "Standalone Rejections", 680, 420, 700, 300);
    pref_win_ = std::make_shared<Window>(ctx_, "preferences", "Preferences", 200, 200, 300, 250);

    md_win_->set_hide_on_minimize(true);
    orders_win_->set_hide_on_minimize(true);
    exec_win_->set_hide_on_minimize(true);
    rej_win_->set_hide_on_minimize(true);
    pref_win_->set_hide_on_minimize(true);

    md_win_->set_destroy_on_close(false);
    orders_win_->set_destroy_on_close(false);
    exec_win_->set_destroy_on_close(false);
    rej_win_->set_destroy_on_close(false);
    pref_win_->set_destroy_on_close(false);

    main_div->append_child(md_win_);
    main_div->append_child(orders_win_);
    main_div->append_child(exec_win_);
    main_div->append_child(rej_win_);
    main_div->append_child(pref_win_);

    md_win_->set_visible(false);
    orders_win_->set_visible(false);
    exec_win_->set_visible(false);
    rej_win_->set_visible(false);
    pref_win_->set_visible(false);

    auto update_taskbar = [this]() {
        // Find existing taskbar or create it
        if (!taskbar_) {
            taskbar_ = std::make_shared<HTMLDivElement>(ctx_);
            ctx_->register_element(taskbar_);
            taskbar_->set_attribute(ctx_->strings.style, "display: flex; gap: 8px; margin-left: 20px;");
            status_bar->append_child(taskbar_);
        }
        taskbar_->set_text_content(ctx_->register_string("")); // Clear it
        
        auto add_taskbar_btn = [&](std::shared_ptr<Window> w, const std::string& label) {
            if (w && w->is_minimized()) {
                auto btn = std::make_shared<Element>(ctx_, ctx_->strings.button);
                ctx_->register_element(btn);
                btn->set_text_content(label);
                btn->set_attribute(ctx_->strings.style, "background: #5c6bc0; color: white; border: none; padding: 4px 8px; font-size: 11px; border-radius: 4px; cursor: pointer;");
                btn->add_event_listener(ctx_->strings.click, [w](const Event&) {
                    w->set_minimized(false);
                });
                taskbar_->append_child(btn);
            }
        };
        add_taskbar_btn(md_win_, "Market Data");
        add_taskbar_btn(orders_win_, "Orders");
        add_taskbar_btn(exec_win_, "Executions");
        add_taskbar_btn(rej_win_, "Rejects");
        add_taskbar_btn(pref_win_, "Prefs");
    };

    // removed set_min_cb
    
    md_win_->set_close_button_hidden(true);
    orders_win_->set_close_button_hidden(true);
    exec_win_->set_close_button_hidden(true);
    rej_win_->set_close_button_hidden(true);
    pref_win_->set_modal(true);
    pref_win_->set_maximize_button_hidden(true);
    pref_win_->set_minimize_button_hidden(true);

    auto pref_cont = pref_win_->get_content_container();
    pref_cont->set_attribute(ctx_->strings.style, "padding: 10px; gap: 10px; flex-grow: 1; min-height: 0; box-sizing: border-box; overflow: auto; display: flex; flex-direction: column;");

    auto create_pref_row = [this, pref_cont](const std::string& label, double val, double step, int prec, auto on_change) {
        auto row = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(row);
        row->set_attribute(ctx_->strings.style, "display: flex; justify-content: space-between; align-items: center; margin-bottom: 5px;");
        
        auto lbl = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
        ctx_->register_element(lbl);
        lbl->set_text_content(label);
        row->append_child(lbl);

        auto input = std::make_shared<HTMLInputElement>(ctx_);
        ctx_->register_element(input);
        input->set_attribute(ctx_->strings.type, "text");
        std::stringstream ss;
        if (prec == 0) ss << (long long)val;
        else ss << std::fixed << std::setprecision(prec) << val;
        input->set_value(ss.str());
        
        input->set_attribute(ctx_->strings.style, "padding: 4px; border: 1px solid #ccc; border-radius: 4px; width: 80px; text-align: right; box-sizing: border-box;");

        auto on_input_change = [input, on_change, this](const Event&) {
            std::string str = input->get_value();
            try {
                size_t idx;
                double v = std::stod(str, &idx);
                if (idx == str.length() && v >= 0) {
                    on_change(v);
                    input->set_style("border-color", "#ccc");
                } else {
                    show_alert("Invalid value entered for preference.");
                    input->set_style("border-color", "red");
                }
            } catch (...) {
                show_alert("Invalid value entered for preference.");
                input->set_style("border-color", "red");
            }
        };

        input->add_event_listener("change", on_input_change);
        
        row->append_child(input);
        pref_cont->append_child(row);
        return input;
    };

    pref_qty_input_ = create_pref_row("Default Qty", pref_default_qty_, 100, 0, [this](double v){ pref_default_qty_ = (int)v; save_layout(); });
    pref_agg_input_ = create_pref_row("Max Aggressive %", pref_max_aggressive_, 0.01, 2, [this](double v){ pref_max_aggressive_ = v; save_layout(); });
    pref_not_input_ = create_pref_row("Max Notional", pref_max_notional_, 100, 0, [this](double v){ pref_max_notional_ = v; save_layout(); });

    auto reset_btn = std::make_shared<HTMLButtonElement>(ctx_);
    ctx_->register_element(reset_btn);
    reset_btn->set_text_content(ctx_->register_string("Reset Window Positions"));
    reset_btn->set_attribute(ctx_->strings.style, "background: #f44336; color: white; border: none; padding: 6px; font-size: 12px; border-radius: 4px; cursor: pointer; margin-top: 10px; font-weight: bold;");
    reset_btn->add_event_listener(ctx_->strings.click, [this](const Event&) {
        // Reset positions
        md_win_->set_bounds(50, 50, 600, 500);
        orders_win_->set_bounds(680, 50, 700, 350);
        exec_win_->set_bounds(50, 420, 600, 300);
        rej_win_->set_bounds(680, 420, 700, 300);
        pref_win_->set_bounds(200, 200, 300, 250);
        save_layout();
    });
    pref_cont->append_child(reset_btn);


    auto save_cb = [this](int, int) { save_layout(); };
    auto state_cb = [this, update_taskbar](bool) { update_taskbar(); save_layout(); };
    md_win_->set_on_move(save_cb); md_win_->set_on_resize(save_cb); md_win_->set_on_minimize(state_cb); md_win_->set_on_maximize(state_cb);
    orders_win_->set_on_move(save_cb); orders_win_->set_on_resize(save_cb); orders_win_->set_on_minimize(state_cb); orders_win_->set_on_maximize(state_cb);
    exec_win_->set_on_move(save_cb); exec_win_->set_on_resize(save_cb); exec_win_->set_on_minimize(state_cb); exec_win_->set_on_maximize(state_cb);
    rej_win_->set_on_move(save_cb); rej_win_->set_on_resize(save_cb); rej_win_->set_on_minimize(state_cb); rej_win_->set_on_maximize(state_cb);
    pref_win_->set_on_move(save_cb); pref_win_->set_on_resize(save_cb); pref_win_->set_on_minimize(state_cb); pref_win_->set_on_maximize(state_cb);


    auto md_cont = md_win_->get_content_container();
    md_cont->set_attribute(ctx_->strings.style, "padding: 0px; flex-grow: 1; min-height: 0; box-sizing: border-box; overflow: auto; display: flex; flex-direction: column;");

    md_grid_ = std::make_shared<Grid<OdysseyMarketData>>(ctx_);
    md_grid_->add_column("Symbol", 80);
    md_grid_->sort_cmp_ = [](const OdysseyMarketData& a, const OdysseyMarketData& b, int col_idx) {
        if (col_idx == 0) return a.symbol < b.symbol;
        if (col_idx == 1) return a.bidSz < b.bidSz;
        if (col_idx == 2) return a.bidPx < b.bidPx;
        if (col_idx == 3) return a.askPx < b.askPx;
        if (col_idx == 4) return a.askSz < b.askSz;
        if (col_idx == 5) return a.lastSz < b.lastSz;
        if (col_idx == 6) return a.lastPx < b.lastPx;
        if (col_idx == 7) return a.volume < b.volume;
        return false;
    };
    md_grid_->add_column("Bid Size", 70);
    md_grid_->add_column("Bid", 80);
    md_grid_->add_column("Ask", 80);
    md_grid_->add_column("Ask Size", 70);
    md_grid_->add_column("Trd Volume", 80);
    md_grid_->add_column("Last", 80);
    md_grid_->add_column("Tot Volume", 90);
    md_grid_->set_on_render_row([this](std::shared_ptr<GridRow<OdysseyMarketData>> row, const OdysseyMarketData& data) {
        row->add_cell("", md_grid_->get_col_width(0), "Symbol");
        
        auto cell = row->get_cell(0);
        cell->set_attribute("style", "padding: 0; box-sizing: border-box; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; user-select: none; width: " + std::to_string(md_grid_->get_col_width(0)) + "px; min-width: " + std::to_string(md_grid_->get_col_width(0)) + "px; max-width: " + std::to_string(md_grid_->get_col_width(0)) + "px;");
        
        auto input = std::make_shared<HTMLInputElement>(ctx_);
        ctx_->register_element(input);
        input->set_value(data.symbol);
        input->set_attribute(ctx_->strings.style, "width: 100%; height: 100%; padding: 6px; border: none; background: transparent; outline: none; font-size: inherit; font-family: inherit; text-transform: uppercase; box-sizing: border-box;");
        cell->append_child(input);
        
        auto handle_edit = [this, row, input]() {
            std::string new_sym = input->get_value();
            std::transform(new_sym.begin(), new_sym.end(), new_sym.begin(), ::toupper);
            std::string old_sym = row->get_data().symbol;
            
            if (new_sym == old_sym) {
                input->set_value(old_sym);
                return;
            }
            
            if (new_sym.empty()) {
                if (!old_sym.empty()) {
                    int count = 0;
                    for (const auto& m : market_data_) {
                        if (m.symbol == old_sym) count++;
                    }
                    if (count <= 1) unsubscribe_symbol(old_sym);
                }
                for (auto it = market_data_.begin(); it != market_data_.end(); ++it) {
                    if (it->row_ptr.get() == row.get()) {
                        market_data_.erase(it);
                        break;
                    }
                }
                rebuild_market_data_grid();
                save_layout();
            } else {
                if (!old_sym.empty()) {
                    int count = 0;
                    for (const auto& m : market_data_) {
                        if (m.symbol == old_sym) count++;
                    }
                    if (count <= 1) unsubscribe_symbol(old_sym);
                }
                
                auto md = row->get_data();
                md.symbol = new_sym;
                md.is_invalid = false;
                row->update_data(md);
                
                for (auto &m : market_data_) {
                    if (m.row_ptr.get() == row.get()) {
                        m.symbol = new_sym;
                        m.is_invalid = false;
                        break;
                    }
                }
                
                subscribe_symbol(new_sym);
                
                bool has_empty = false;
                for (const auto& m : market_data_) {
                    if (m.symbol.empty()) has_empty = true;
                }
                if (!has_empty) {
                    market_data_.push_back(OdysseyMarketData{"", 0, 0, 0, 0, 0, 0, 0, nullptr, false});
                    rebuild_market_data_grid();
                }
                save_layout();
            }
        };
        
        input->add_event_listener("focus", [input](const Event& e) {
            input->set_style_conflated("box-shadow", "inset 0 0 0 2px darkslateblue");
        });
        
        input->add_event_listener("blur", [handle_edit, input](const Event& e) {
            input->set_style_conflated("box-shadow", "none");
            handle_edit();
        });
        
        input->add_event_listener("keydown", [handle_edit](const Event& e) {
            auto* ke = dynamic_cast<const KeyboardEvent*>(&e);
            if (ke && ke->get_key() == "Enter") {
                handle_edit();
            }
        });
        
        if (data.is_invalid) {
            auto invalid_cell = std::make_shared<HTMLDivElement>(ctx_);
            ctx_->register_element(invalid_cell);
            invalid_cell->set_text_content("Symbol is invalid - enter order and try again");
            invalid_cell->set_attribute(ctx_->strings.style, "padding: 6px; flex: 1; color: red; font-weight: bold; overflow: hidden; white-space: nowrap; user-select: none; border: 2px solid red; text-align:center;");
            row->append_child(invalid_cell);
        } else if (data.symbol.empty()) {
            row->add_cell("", md_grid_->get_col_width(1));
            row->add_cell("", md_grid_->get_col_width(2));
            row->add_cell("", md_grid_->get_col_width(3));
            row->add_cell("", md_grid_->get_col_width(4));
            row->add_cell("", md_grid_->get_col_width(5));
            row->add_cell("", md_grid_->get_col_width(6));
            row->add_cell("", md_grid_->get_col_width(7));
        } else {
            row->add_cell(std::to_string(data.bidSz), md_grid_->get_col_width(1), "Bid Size");
            row->add_cell(format_px(data.bidPx), md_grid_->get_col_width(2), "Bid");
            row->add_cell(format_px(data.askPx), md_grid_->get_col_width(3), "Ask");
            row->add_cell(std::to_string(data.askSz), md_grid_->get_col_width(4), "Ask Size");
            row->add_cell(std::to_string(data.lastSz), md_grid_->get_col_width(5), "Trd Volume");
            row->add_cell(format_px(data.lastPx), md_grid_->get_col_width(6), "Last");
            row->add_cell(std::to_string(data.volume), md_grid_->get_col_width(7), "Tot Volume");
        }
    });
    md_cont->append_child(md_grid_);
    md_grid_->set_on_cell_double_click([this](auto row, const std::string& col) {
        std::string symbol = row->get_data().symbol;
        std::string side = "Buy";
        double price = row->get_data().lastPx;
        uint32_t qty = pref_default_qty_;
        std::string focus_field = "";

        if (col == "Bid" || col == "Bid Size") {
            if (col == "Bid Size")
            {
              side = "Sell";
              qty = row->get_data().bidSz;
              focus_field = "qty";
            } else {
              side = "Buy";
              qty = pref_default_qty_;
              focus_field = "price";
            }
            price = row->get_data().bidPx;
            
        } else if (col == "Ask" || col == "Ask Size") {
            if (col == "Ask Size") {
              side = "Buy";
              qty = row->get_data().askSz;
              focus_field = "qty";
            } else {
              side = "Sell";
              qty = pref_default_qty_;
              focus_field = "price";
            }
            price = row->get_data().askPx;
        }
        else {
          return;
        }
        open_order_entry_symbol(symbol, side, price, qty, focus_field);
    });

    auto orders_cont = orders_win_->get_content_container();
    orders_cont->set_attribute(ctx_->strings.style, "padding: 0px; flex-grow: 1; min-height: 0; box-sizing: border-box; overflow: auto; display: flex; flex-direction: column;");

    auto btns = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(btns);
    btns->set_attribute(ctx_->strings.style, "display: flex; gap: 8px; margin: 12px; flex-shrink: 0;");
    orders_cont->append_child(btns);

    auto add_btn_func = [this, btns](const std::string& label, const std::string& bg_color, auto cb) {
        auto btn = std::make_shared<Button>(ctx_);
        ctx_->register_element(btn);
        btn->set_text(label);
        btn->set_color(bg_color);
        btn->set_text_color("white");
        btn->set_extra_style("font-size: 12px; font-weight: bold; border-radius: 6px;");
        btn->add_event_listener(ctx_->strings.click, cb);
        btns->append_child(btn);
    };

    add_btn_func("New Buy Order", "#2e7d32", [this](const Event&) { open_order_entry("Buy"); });
    add_btn_func("New Sell Order", "#c62828", [this](const Event&) { open_order_entry("Sell"); });
    add_btn_func("Cancel", "#e65100", [this](const Event&) { cancel_selected_order(); });
    add_btn_func("Replace", "#1a237e", [this](const Event&) {
        if (!selected_order_id_.empty()) {
            for (const auto &ord : orders_) {
                if (ord.id == selected_order_id_ && (ord.status == "New" || ord.status == "Partially filled" || ord.status == "Replaced")) {
                    open_replace_entry(ord); break;
                }
            }
        }
    });
    add_btn_func("Cancel Buys", "#558b2f", [this](const Event&) { cancel_all_orders_side("Buy"); });
    add_btn_func("Cancel Sells", "#ad1457", [this](const Event&) { cancel_all_orders_side("Sell"); });

    order_grid_ = std::make_shared<Grid<OdysseyOrderData>>(ctx_);
    order_grid_->add_column("Ord ID", 100);
    order_grid_->sort_cmp_ = [](const OdysseyOrderData& a, const OdysseyOrderData& b, int col_idx) {
        if (col_idx == 0) return a.id < b.id;
        if (col_idx == 1) return a.symbol < b.symbol;
        if (col_idx == 2) return a.side < b.side;
        if (col_idx == 3) return a.price < b.price;
        if (col_idx == 4) return a.qty < b.qty;
        if (col_idx == 5) return a.cumQty < b.cumQty;
        if (col_idx == 6) return a.leaves < b.leaves;
        if (col_idx == 7) return a.status < b.status;
        return false;
    };
    order_grid_->add_column("Symbol", 80);
    order_grid_->add_column("Side", 60);
    order_grid_->add_column("Px", 70);
    order_grid_->add_column("Qty", 70);
    order_grid_->add_column("CumQty", 70);
    order_grid_->add_column("Leaves", 70);
    order_grid_->add_column("Status", 100);
    order_grid_->add_column("Orig ID", 100);
    order_grid_->set_on_render_row([this](std::shared_ptr<GridRow<OdysseyOrderData>> row, const OdysseyOrderData& ord) {
        row->add_cell(ord.id, order_grid_->get_col_width(0));
        row->add_cell(ord.symbol, order_grid_->get_col_width(1));
        row->add_cell(ord.side, order_grid_->get_col_width(2));
        row->add_cell(format_px(ord.price), order_grid_->get_col_width(3));
        row->add_cell(std::to_string(ord.qty), order_grid_->get_col_width(4));
        row->add_cell(std::to_string(ord.cumQty), order_grid_->get_col_width(5));
        row->add_cell(std::to_string(ord.leaves), order_grid_->get_col_width(6));
        row->add_cell(ord.status, order_grid_->get_col_width(7));
        row->add_cell("", order_grid_->get_col_width(8));
        
        row->add_event_listener(ctx_->strings.click, [this, ord_id = ord.id, r = row](const Event&) {
            if (order_row_)
            {
               order_row_->set_attribute(ctx_->strings.style, "display: flex; border-bottom: 1px solid rgba(0,0,0,0.05); width: 100%;");
            }
            selected_order_id_ = ord_id;
            order_row_ = r;
            // Since GridRow inherits HTMLDivElement, we can set style
            // Note: simple highlight
            r->set_attribute(ctx_->strings.style, "display: flex; border-bottom: 1px solid rgba(0,0,0,0.05); width: 100%; background: #e3f2fd;");
        });
    });
    orders_cont->append_child(order_grid_);
    
    exec_grid_ = std::make_shared<Grid<OdysseyExecutionData>>(ctx_);
    exec_grid_->add_column("Exec ID", 100);
    exec_grid_->sort_cmp_ = [](const OdysseyExecutionData& a, const OdysseyExecutionData& b, int col_idx) {
        if (col_idx == 0 || col_idx == 1) return a.id < b.id;
        if (col_idx == 2) return a.symbol < b.symbol;
        if (col_idx == 3) return a.side < b.side;
        if (col_idx == 4) return a.lastSz < b.lastSz;
        if (col_idx == 5) return a.lastPx < b.lastPx;
        return false;
    };
    exec_grid_->add_column("Ord ID", 100);
    exec_grid_->add_column("Symbol", 80);
    exec_grid_->add_column("Side", 60);
    exec_grid_->add_column("Last Qty", 80);
    exec_grid_->add_column("Last Px", 80);
    exec_grid_->set_on_render_row([this](std::shared_ptr<GridRow<OdysseyExecutionData>> row, const OdysseyExecutionData& ex) {
        row->add_cell(ex.id, exec_grid_->get_col_width(0));
        row->add_cell(ex.id, exec_grid_->get_col_width(1));
        row->add_cell(ex.symbol, exec_grid_->get_col_width(2));
        row->add_cell(ex.side, exec_grid_->get_col_width(3));
        row->add_cell(std::to_string(ex.lastSz), exec_grid_->get_col_width(4));
        row->add_cell(format_px(ex.lastPx), exec_grid_->get_col_width(5));
    });
    exec_win_->get_content_container()->append_child(exec_grid_);

    reject_grid_ = std::make_shared<Grid<OdysseyRejectData>>(ctx_);
    reject_grid_->add_column("ID", 100);
    reject_grid_->sort_cmp_ = [](const OdysseyRejectData& a, const OdysseyRejectData& b, int col_idx) {
        if (col_idx == 0 || col_idx == 1) return a.id < b.id;
        if (col_idx == 2) return a.refMsgType < b.refMsgType;
        if (col_idx == 3) return a.status < b.status;
        if (col_idx == 4) return a.text < b.text;
        return false;
    };
    reject_grid_->add_column("Orig ID", 100);
    reject_grid_->add_column("Msg Type", 120);
    reject_grid_->add_column("Status", 100);
    reject_grid_->add_column("Text", 200);
    reject_grid_->set_on_render_row([this](std::shared_ptr<GridRow<OdysseyRejectData>> row, const OdysseyRejectData& rej) {
        row->add_cell(rej.id, reject_grid_->get_col_width(0));
        row->add_cell("", reject_grid_->get_col_width(1));
        row->add_cell(rej.refMsgType, reject_grid_->get_col_width(2));
        row->add_cell(rej.status, reject_grid_->get_col_width(3));
        row->add_cell(rej.text, reject_grid_->get_col_width(4));
    });
    rej_win_->get_content_container()->append_child(reject_grid_);

        rebuild_market_data_grid();
    rebuild_orders_grid();
    rebuild_executions_grid();
    rebuild_rejections_grid();
}

  

  

  void strncpy_safe(char *dst, const char *src, size_t dst_size) {
    std::strncpy(dst, src, dst_size);
    dst[dst_size - 1] = '\0';
  }

  

  

  void rebuild_orders_tabs() {}

  void apply_tab_style(std::shared_ptr<Element> tab, bool active) {
    tab->set_attribute(ctx_->strings.style,
                       "padding: 6px 12px; margin-right: 6px; border-radius: "
                       "6px; cursor: pointer; "
                       "font-size: 13px; font-weight: bold; border: 1px solid "
                       "rgba(0,0,0,0.06); "
                       "background: " +
                           std::string(active
                                           ? "#1a237e; color: #fff;"
                                           : "rgba(0,0,0,0.04); color: #555;"));
  }

  

  

  

  

  void rebuild_market_data_grid() {
    md_grid_->clear_rows();
    for (auto& md : market_data_) md.row_ptr = md_grid_->add_row(md);
    md_grid_->apply_sort();
  }

  void rebuild_orders_grid() {
    order_grid_->clear_rows();
    for (const auto& ord : orders_) order_grid_->add_row(ord);
    order_grid_->apply_sort();
}

  void rebuild_executions_grid() {
    exec_grid_->clear_rows();
    for (const auto& exec : executions_) exec_grid_->add_row(exec);
    exec_grid_->apply_sort();
}

  void rebuild_rejections_grid() {
    reject_grid_->clear_rows();
    for (const auto& rej : rejections_) reject_grid_->add_row(rej);
    reject_grid_->apply_sort();
}

  

  std::string format_px(double px) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << px;
    return ss.str();
  }

  void render_logon_prompt() {
    logon_prompt = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(logon_prompt);
    logon_prompt->set_attribute(
        ctx_->strings.style,
        "position: absolute; left: 50%; top: 50%; transform: translate(-50%, "
        "-50%); "
        "width: 320px; padding: 30px; background: rgb(240, 240, 240); "
        "border-radius: 16px; border: 1px solid rgba(255, 255, 255, 0.4); "
        "box-shadow: 0 10px 40px 0 rgba(31, 38, 135, 0.2); "
        "display: flex; flex-direction: column; gap: 15px; align-items: "
        "center; z-index: 2000;");
    main_div->append_child(logon_prompt);

    auto header_container = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(header_container);
    header_container->set_attribute(
        ctx_->strings.style,
        "display: flex; align-items: center; justify-content: center; gap: 10px; margin-bottom: 5px;");
    logon_prompt->append_child(header_container);

    auto logo_img = std::make_shared<HTMLImageElement>(ctx_);
    ctx_->register_element(logo_img);
    logo_img->set_src("/logo.png");
    logo_img->set_attribute(ctx_->strings.style, "height: 32px; object-fit: contain;");
    header_container->append_child(logo_img);

    auto logo = std::make_shared<Element>(ctx_, ctx_->strings.h3);
    ctx_->register_element(logo);
    logo->set_text_content(ctx_->register_string("Odyssey Trader"));
    logo->set_attribute(
        ctx_->strings.style,
        "margin: 0; color: #1a237e; font-weight: 800; font-size: 22px;");
    header_container->append_child(logo);

    username_input = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(username_input);
    username_input->set_attribute(ctx_->strings.placeholder, "Enter Username");
    username_input->set_attribute(
        ctx_->strings.style,
        "width: 100%; padding: 10px; border-radius: 8px; border: 1px solid "
        "#ccc; "
        "font-size: 14px; text-align: center; box-sizing: border-box;");
    logon_prompt->append_child(username_input);

    auto logon_btn = std::make_shared<Button>(ctx_);
    ctx_->register_element(logon_btn);
    logon_btn->set_text("Logon");
    logon_btn->set_color("#1a237e");
    logon_btn->set_text_color("white");
    logon_btn->set_extra_style("width: 100%; font-size: 14px; font-weight: bold; border-radius: 8px; transition: 0.2s;");
    logon_prompt->append_child(logon_btn);

    auto do_logon = [this]() {
      std::string user = username_input->get_value();
      if (!user.empty()) {
        username_ = user;
        MAIN_THREAD_EM_ASM({
          document.body.requestFullscreen().catch(
              err => { console.log("Fullscreen request failed:", err); });
        });

        logon_prompt->set_attribute(ctx_->strings.style, "display: none;");
        status_bar_visible_ = true;
        updateStatusBar();

        send_logon_req();
      }
    };

    logon_btn->add_event_listener(ctx_->strings.click, [do_logon](const Event &e) { do_logon(); });
    username_input->add_event_listener("keydown", [do_logon](const Event& e) {
        if (const KeyboardEvent* ke = dynamic_cast<const KeyboardEvent*>(&e)) {
            if (ke->get_key() == "Enter") {
                do_logon();
            }
        }
    });

    order_dialog = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(order_dialog);
    apply_order_dialog_style();
    main_div->append_child(order_dialog);

    order_dialog_title = std::make_shared<Element>(ctx_, ctx_->strings.h4);
    ctx_->register_element(order_dialog_title);
    order_dialog_title->set_text_content(ctx_->register_string("Order Entry"));
    order_dialog_title->set_attribute(ctx_->strings.style,
                                      "margin: 0; color: #1a237e;");
    order_dialog->append_child(order_dialog_title);

    order_symbol_input = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(order_symbol_input);
    order_symbol_input->set_attribute(
        ctx_->strings.style, "width: 100%; padding: 6px; border-radius: 6px;");
    order_symbol_input->set_attribute(ctx_->strings.placeholder, "Symbol");
    order_symbol_input->set_attribute(ctx_->strings.type, "text");
    order_dialog->append_child(order_symbol_input);

    order_qty_input = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(order_qty_input);
    order_qty_input->set_attribute(ctx_->strings.placeholder, "Quantity");
    order_qty_input->set_attribute(ctx_->strings.type, "number"); 
    order_qty_input->set_attribute(ctx_->register_string("step"), "100");
    order_qty_input->set_numeric_only(false);
    order_qty_input->set_attribute(
        ctx_->strings.style, "width: 100%; padding: 6px; border-radius: 6px; "
                             "border: 1px solid #ccc; box-sizing: border-box;");
    order_dialog->append_child(order_qty_input);

    order_price_input = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(order_price_input);
    order_price_input->set_attribute(ctx_->strings.placeholder, "Price");
    order_price_input->set_attribute(ctx_->strings.type, "number");
    order_price_input->set_attribute(ctx_->register_string("step"), "0.01");
    order_price_input->set_numeric_only(true);
    order_price_input->set_attribute(
        ctx_->strings.style, "width: 100%; padding: 6px; border-radius: 6px; "
                             "border: 1px solid #ccc; box-sizing: border-box;");
    order_dialog->append_child(order_price_input);

    auto handler = [this](const Event& e) {
        if (const KeyboardEvent* ke = dynamic_cast<const KeyboardEvent*>(&e)) {
            if (ke->get_key() == "Enter") submit_order();
        }
    };
    order_symbol_input->add_event_listener("keydown", handler);
    order_qty_input->add_event_listener("keydown", handler);
    order_price_input->add_event_listener("keydown", handler);

    auto dlg_btns = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(dlg_btns);
    dlg_btns->set_attribute(
        ctx_->strings.style,
        "display: flex; gap: 8px; justify-content: flex-end;");
    order_dialog->append_child(dlg_btns);

    auto dlg_cancel = std::make_shared<Button>(ctx_);
    ctx_->register_element(dlg_cancel);
    dlg_cancel->set_text("Cancel");
    dlg_cancel->set_color("#eee");
    dlg_cancel->set_text_color("#333");
    dlg_cancel->set_extra_style("border-radius: 6px;");
    dlg_cancel->add_event_listener(ctx_->strings.click, [this](const Event &e) {
      order_dialog_visible_ = false;
      apply_order_dialog_style();
    });
    dlg_btns->append_child(dlg_cancel);

    order_dialog_submit_btn = std::make_shared<Button>(ctx_);
    ctx_->register_element(order_dialog_submit_btn);
    order_dialog_submit_btn->set_text("Submit");
    order_dialog_submit_btn->set_color("#1a237e");
    order_dialog_submit_btn->set_text_color("white");
    order_dialog_submit_btn->set_extra_style("font-weight: bold; border-radius: 6px;");
    order_dialog_submit_btn->add_event_listener(ctx_->strings.click,
                                   [this](const Event &e) { submit_order(); });
    dlg_btns->append_child(order_dialog_submit_btn);
  }

  void apply_order_dialog_style() {
    std::stringstream ss;
    ss << "position: absolute; left: 50%; top: 50%; transform: translate(-50%, "
          "-50%); "
       << "width: 300px; padding: 20px; background: rgba(255,255,255,0.95); "
       << "border-radius: 12px; border: 1px solid rgba(0,0,0,0.15); "
       << "box-shadow: 0 8px 30px rgba(0,0,0,0.25); "
       << "display: " << (order_dialog_visible_ ? "flex" : "none") << "; "
       << "flex-direction: column; gap: 12px; z-index: 2500;";
    order_dialog->set_attribute(ctx_->strings.style, ss.str());
  }

  void open_order_entry(const std::string &side) {
    order_dialog_side = side;
    replace_order_id = "";

    order_symbol_input->set_disabled(false);
    order_symbol_input->set_value("");
    order_qty_input->set_value("");
    order_price_input->set_value("");

    std::stringstream ss;
    ss << side << " Order Entry";
    order_dialog_title->set_text_content(ss.str());

    order_dialog_visible_ = true;
    apply_order_dialog_style();
  }

  void open_order_entry_symbol(const std::string &symbol, const std::string &side, double price = 0.0, uint32_t qty = 0, const std::string& focus_field = "") {
    open_order_entry(side);
    order_symbol_input->set_value(symbol);
    if (price > 0.0) {
        order_price_input->set_value(format_px(price));
    }
    if (qty > 0) {
        order_qty_input->set_value(std::to_string(qty));
    } else {
        order_qty_input->set_value(std::to_string(pref_default_qty_));
    }

    if (focus_field == "qty") {
        order_qty_input->focus();
        order_qty_input->select();
    } else if (focus_field == "price") {
        order_price_input->focus();
        order_price_input->select();
    }
}

  void open_replace_entry(const OdysseyOrderData &ord) {
    order_dialog_side = ord.side;
    replace_order_id = ord.id;

    order_symbol_input->set_value(ord.symbol);
    order_symbol_input->set_disabled(true);
    order_qty_input->set_value(std::to_string(ord.qty));
    order_price_input->set_value(format_px(ord.price));

    std::stringstream ss;
    ss << "Modify " << ord.side << " Order (ID: " << ord.id << ")";
    order_dialog_title->set_text_content(ss.str());

    order_dialog_visible_ = true;
    apply_order_dialog_style();
  }

  void execute_submit(const std::string& symbol, const std::string& side, double price, uint32_t qty) {
    order_dialog_visible_ = false;
    apply_order_dialog_style();

    sender_seq_num_++;
    updateStatusBar();

    if (replace_order_id.empty()) {
      NetMsg<NewOrderMsg> msg;
      msg.header.type = htons(static_cast<uint16_t>(MsgType::NewOrder));
      msg.header.length = htonl(sizeof(msg));
      std::string new_cl_ord_id = generateOrderId();
      strncpy_safe(msg.payload.cl_ord_id, new_cl_ord_id.c_str(), sizeof(msg.payload.cl_ord_id));
      strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
      msg.payload.side = (side == "Buy") ? 1 : 2;
      msg.payload.type = 2; // Limit
      msg.payload.price = price;
      msg.payload.qty = qty;
      strncpy_safe(msg.payload.username, username_.c_str(), sizeof(msg.payload.username));

      send_binary_msg(&msg, sizeof(msg));
    } else {
      NetMsg<OrderReplaceMsg> msg;
      msg.header.type = htons(static_cast<uint16_t>(MsgType::OrderReplace));
      msg.header.length = htonl(sizeof(msg));
      strncpy_safe(msg.payload.orig_cl_ord_id, replace_order_id.c_str(), sizeof(msg.payload.orig_cl_ord_id));
      std::string new_cl_ord_id = generateOrderId();
      strncpy_safe(msg.payload.new_cl_ord_id, new_cl_ord_id.c_str(), sizeof(msg.payload.new_cl_ord_id));
      strncpy_safe(msg.payload.symbol, symbol.c_str(), sizeof(msg.payload.symbol));
      msg.payload.side = (side == "Buy") ? 1 : 2;
      msg.payload.price = price;
      msg.payload.qty = qty;
      strncpy_safe(msg.payload.username, username_.c_str(), sizeof(msg.payload.username));

      send_binary_msg(&msg, sizeof(msg));
    }
}

  void submit_order() {
    std::string symbol = order_symbol_input->get_value();
    std::string qty_str = order_qty_input->get_value();
    std::string px_str = order_price_input->get_value();

    if (symbol.empty() || qty_str.empty() || px_str.empty()) return;

    uint32_t qty = 0;
    double price = 0.0;
    try {
      qty = std::stoul(qty_str);
      price = std::stod(px_str);
      if (qty <= 0 || price <= 0.0) {
        show_alert("Quantity and Price must be greater than zero.");
        return;
      }
    } catch (...) {
      show_alert("Invalid quantity or price format.");
      return;
    }
    
    std::string side = order_dialog_side;
    
    bool needs_confirm = false;
    std::string confirm_msg = "";

    double notional = price * qty;
    if (notional > pref_max_notional_) {
        needs_confirm = true;
        confirm_msg += "Order exceeds Max Notional (" + std::to_string(static_cast<uint64_t>(pref_max_notional_)) + "). ";
    }

    for (auto& md : market_data_) {
        if (md.symbol == symbol) {
            if (side == "Buy" && md.askPx > 0) {
                double pct_through = ((price - md.askPx) / md.askPx) * 100.0;
                if (pct_through > pref_max_aggressive_) {
                    needs_confirm = true;
                    confirm_msg += "Buy order is too aggressive (" + std::to_string(pct_through) + "% > " + std::to_string(pref_max_aggressive_) + "%). ";
                }
            } else if (side == "Sell" && md.bidPx > 0) {
                double pct_through = ((md.bidPx - price) / md.bidPx) * 100.0;
                if (pct_through > pref_max_aggressive_) {
                    needs_confirm = true;
                    confirm_msg += "Sell order is too aggressive (" + std::to_string(pct_through) + "% > " + std::to_string(pref_max_aggressive_) + "%). ";
                }
            }
            break;
        }
    }

    if (needs_confirm) {
        auto confirm_win = std::make_shared<Window>(ctx_, "confirm_new", "Risk Warning", 400, 300, 400, 200);
        confirm_win->set_modal(true);
        main_div->append_child(confirm_win);
        
        auto msg_el = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(msg_el);
        msg_el->set_text_content("Warning: " + confirm_msg + " Submit anyway?");
        msg_el->set_attribute(ctx_->strings.style, "padding: 20px; font-weight: bold; color: #d32f2f;");
        confirm_win->get_content_container()->append_child(msg_el);

        auto btn_yes = std::make_shared<Button>(ctx_);
        ctx_->register_element(btn_yes);
        btn_yes->set_text("Yes, Submit");
        btn_yes->set_color("#1a237e");
        btn_yes->set_text_color("white");
        btn_yes->set_extra_style("font-size: 13px; font-weight: bold; border-radius: 6px;");
        btn_yes->add_event_listener(ctx_->strings.click, [this, confirm_win, symbol, side, price, qty](const Event&) {
            execute_submit(symbol, side, price, qty);
            confirm_win->set_visible(false);
        });
        
        auto btn_no = std::make_shared<Button>(ctx_);
        ctx_->register_element(btn_no);
        btn_no->set_text("Cancel");
        btn_no->set_color("#eee");
        btn_no->set_text_color("#333");
        btn_no->set_extra_style("font-size: 13px; border-radius: 6px;");
        btn_no->add_event_listener(ctx_->strings.click, [this, confirm_win](const Event&) {
            confirm_win->set_visible(false);
        });

        confirm_win->get_content_container()->append_child(btn_yes);
        confirm_win->get_content_container()->append_child(btn_no);
        return;
    }

    execute_submit(symbol, side, price, qty);
}

  void cancel_selected_order() {
    if (selected_order_id_.empty())
      return;

    for (const auto &ord : orders_) {
      if (ord.id == selected_order_id_ &&
          (ord.status == "New" || ord.status == "Partially filled" || ord.status == "Replaced")) {
        sender_seq_num_++;
        updateStatusBar();

        NetMsg<OrderCancelMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::OrderCancel));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.orig_cl_ord_id, ord.id.c_str(),
                     sizeof(msg.payload.orig_cl_ord_id));
        strncpy_safe(msg.payload.symbol, ord.symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = (ord.side == "Buy") ? 1 : 2;
        strncpy_safe(msg.payload.username, username_.c_str(), sizeof(msg.payload.username));

        send_binary_msg(&msg, sizeof(msg));
        return;
      }
    }
  }

  void cancel_all_orders_side(const std::string &side) {
    for (const auto &ord : orders_) {
      if (ord.side == side &&
          (ord.status == "New" || ord.status == "Partially filled" || ord.status == "Replaced")) {
        sender_seq_num_++;
        updateStatusBar();

        NetMsg<OrderCancelMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::OrderCancel));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.orig_cl_ord_id, ord.id.c_str(),
                     sizeof(msg.payload.orig_cl_ord_id));
        strncpy_safe(msg.payload.symbol, ord.symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = (ord.side == "Buy") ? 1 : 2;
        strncpy_safe(msg.payload.username, username_.c_str(), sizeof(msg.payload.username));

        send_binary_msg(&msg, sizeof(msg));
      }
    }
  }

  void cancel_all_orders() {
    for (const auto &ord : orders_) {
      if (ord.status == "New" || ord.status == "Partially filled" || ord.status == "Replaced") {
        sender_seq_num_++;
        updateStatusBar();

        NetMsg<OrderCancelMsg> msg;
        msg.header.type = htons(static_cast<uint16_t>(MsgType::OrderCancel));
        msg.header.length = htonl(sizeof(msg));
        strncpy_safe(msg.payload.orig_cl_ord_id, ord.id.c_str(),
                     sizeof(msg.payload.orig_cl_ord_id));
        strncpy_safe(msg.payload.symbol, ord.symbol.c_str(), sizeof(msg.payload.symbol));
        msg.payload.side = (ord.side == "Buy") ? 1 : 2;
        strncpy_safe(msg.payload.username, username_.c_str(), sizeof(msg.payload.username));

        send_binary_msg(&msg, sizeof(msg));
      }
    }
  }

  void connectWebSocket() {
    if (ws_ > 0) {
      emscripten_websocket_delete(ws_);
      ws_ = 0;
    }

    socketRecvBuf_.clear();

    EmscriptenWebSocketCreateAttributes ws_attrs = {
        "ws://localhost:3000/odyssey", NULL, EM_TRUE};
    ws_ = emscripten_websocket_new(&ws_attrs);
    last_connect_attempt_ = emscripten_get_now();

    emscripten_websocket_set_onopen_callback(
        ws_, this,
        [](int eventType, const EmscriptenWebSocketOpenEvent *event,
           void *userData) -> EM_BOOL {
          OdysseyTraderApp *app = static_cast<OdysseyTraderApp *>(userData);
          app->connected_state = 1;
          std::cout << "Odyssey Proxy WebSocket connected." << std::endl;
          if (!app->username_.empty()) {
            app->send_logon_req();
          }
          return EM_TRUE;
        });

    emscripten_websocket_set_onclose_callback(
        ws_, this,
        [](int eventType, const EmscriptenWebSocketCloseEvent *event,
           void *userData) -> EM_BOOL {
          OdysseyTraderApp *app = static_cast<OdysseyTraderApp *>(userData);
          app->connected_state = 0;
          if (app->ws_ > 0) {
            emscripten_websocket_delete(app->ws_);
            app->ws_ = 0;
          }
          app->last_connect_attempt_ = 0.0;
          std::cout
              << "Odyssey Proxy WebSocket closed. Will reconnect in next tick."
              << std::endl;
          return EM_TRUE;
        });

    emscripten_websocket_set_onerror_callback(
        ws_, this,
        [](int eventType, const EmscriptenWebSocketErrorEvent *event,
           void *userData) -> EM_BOOL {
          OdysseyTraderApp *app = static_cast<OdysseyTraderApp *>(userData);
          app->connected_state = 0;
          if (app->ws_ > 0) {
            emscripten_websocket_delete(app->ws_);
            app->ws_ = 0;
          }
          app->last_connect_attempt_ = 0.0;
          std::cout
              << "Odyssey Proxy WebSocket error. Will reconnect in next tick."
              << std::endl;
          return EM_TRUE;
        });

    emscripten_websocket_set_onmessage_callback(
        ws_, this,
        [](int eventType, const EmscriptenWebSocketMessageEvent *event,
           void *userData) -> EM_BOOL {
          OdysseyTraderApp *app = static_cast<OdysseyTraderApp *>(userData);
          if (event->isText) {
            std::string text(reinterpret_cast<const char *>(event->data),
                             event->numBytes);
            app->handle_text_message(text);
          } else {
            app->handle_binary_message(event->data, event->numBytes);
          }
          return EM_TRUE;
        });
  }

  void send_logon_req() {
    if (ws_ <= 0 || username_.empty())
      return;
    sender_seq_num_++;
    updateStatusBar();
    last_logon_sent_time_ = emscripten_get_now();

    NetMsg<LogonReqMsg> msg;
    msg.header.type = htons(static_cast<uint16_t>(MsgType::LogonReq));
    msg.header.length = htonl(sizeof(msg));
    strncpy_safe(msg.payload.username, username_.c_str(), sizeof(msg.payload.username));
    send_binary_msg(&msg, sizeof(msg));
  }

  void unsubscribe_symbol(const std::string &sym) {
    if (sym.empty()) return;
    subscribed_symbols_.erase(sym);

    NetMsg<MDRequestMsg> msg;
    msg.header.type = htons(static_cast<uint16_t>(MsgType::MDRequest));
    msg.header.length = htonl(sizeof(msg));
    strncpy_safe(msg.payload.symbol, sym.c_str(), sizeof(msg.payload.symbol));
    msg.payload.sub_type = 1;
    send_binary_msg(&msg, sizeof(msg));
  }

  void remove_symbol(const std::string &sym) {
    if (!sym.empty()) unsubscribe_symbol(sym);
    auto it = std::remove_if(market_data_.begin(), market_data_.end(),
                             [&](const OdysseyMarketData& md) { return md.symbol == sym; });
    if (it != market_data_.end()) {
      market_data_.erase(it, market_data_.end());
      rebuild_market_data_grid();
    }
  }

  void subscribe_symbol(const std::string &sym) {
    if (sym.empty()) return;
    subscribed_symbols_.insert(sym);

    NetMsg<MDRequestMsg> msg;
    msg.header.type = htons(static_cast<uint16_t>(MsgType::MDRequest));
    msg.header.length = htonl(sizeof(msg));
    strncpy_safe(msg.payload.symbol, sym.c_str(), sizeof(msg.payload.symbol));
    msg.payload.sub_type = 0;
    send_binary_msg(&msg, sizeof(msg));

    rebuild_market_data_grid();
  }

  void send_binary_msg(const void *body, size_t body_len) {
    if (ws_ <= 0) {
      show_alert("Not connected to server!");
      return;
    }
    const MsgHeader *hdr = static_cast<const MsgHeader *>(body);
    if (ntohs(hdr->type) != static_cast<uint16_t>(MsgType::LogonReq) && connected_state != 2) {
      show_alert("Not logged on! Cannot send message.");
      return;
    }
    emscripten_websocket_send_binary(ws_, const_cast<void*>(body), body_len);
  }

  void handle_text_message(const std::string &text) {
    const std::string prefix = "LAYOUT|" + username_ + "|";
    if (text.find(prefix) == 0) {
      std::string json = text.substr(prefix.size());
      handle_layout_response(json);
    }
  }

  void handle_binary_message(const uint8_t *buf, size_t size) {
    socketRecvBuf_.insert(socketRecvBuf_.end(), buf, buf + size);

    const auto * data = socketRecvBuf_.data();
    auto remaining_size = socketRecvBuf_.size();

    while (remaining_size >= sizeof(MsgHeader)) {
      const auto &hdr = *reinterpret_cast<const MsgHeader*>(data);
      
      uint16_t type = ntohs(hdr.type);
      uint32_t length = ntohl(hdr.length);

      if (remaining_size < length)
        return;

      if (length & 15) [[unlikely]]
      {
        std::cout << "Invalid message length: " << length << " - protocol requires length to be divisible by 16" << std::endl;
        
        if (ws_ > 0) {
            emscripten_websocket_delete(ws_);
            ws_ = 0;
        }
        last_connect_attempt_ = 0.0;
        connected_state = 0;
        updateStatusBar();
        return;
      }

      target_seq_num_++;
      updateStatusBar();

      MsgType msg_type = static_cast<MsgType>(type);

      switch (msg_type) {
      case MsgType::LogonResp: {
        const auto & msg = reinterpret_cast<const NetMsg<LogonRespMsg>*>(data)->payload;
        if (length >= sizeof(msg)) {
          if (msg.status == 0) {
            connected_state = 2;
            std::cout << "Odyssey FIX proxy logon accepted. Session: "
                      << msg.session_name << std::endl;

            md_win_->set_visible(true);
            orders_win_->set_visible(true);
            exec_win_->set_visible(true);
            rej_win_->set_visible(true);

            std::string req = "GET_LAYOUT|" + username_;
            emscripten_websocket_send_utf8_text(ws_, req.c_str());

            subscribe_symbol("LNUX");
            subscribe_symbol("YHOO");
            subscribe_symbol("CSCO");
            
            bool has_empty = false;
            for (const auto& m : market_data_) {
                if (m.symbol.empty()) has_empty = true;
            }
            if (!has_empty) {
                market_data_.push_back(OdysseyMarketData{"", 0, 0, 0, 0, 0, 0, 0, nullptr, false});
                rebuild_market_data_grid();
            }
          }
        }
        break;
      }
      case MsgType::LogonState: {
        const auto& msg = reinterpret_cast<const NetMsg<LogonStateMsg>*>(data)->payload;

          size_t offset = sizeof(NetMsg<LogonStateMsg>);
          orders_.clear();
          orders_map_.clear();
          executions_.clear();
          rejections_.clear();

          for (uint32_t i = 0; i < msg.order_count; ++i) {
            const auto& lso = *reinterpret_cast<const LogonStateOrder*>(data + offset);
            offset += sizeof(lso);

            OdysseyOrderData o;
            o.id = lso.cl_ord_id;
            o.symbol = lso.symbol;
            o.side = (lso.side == 1) ? "Buy" : "Sell";
            o.price = lso.price;
            o.qty = lso.qty;
            o.cumQty = lso.cum_qty;
            o.leaves = lso.qty - lso.cum_qty;
            o.avgPx = lso.price;

            if (lso.status == 0)
              o.status = "New";
            else if (lso.status == 1)
              o.status = "Partially filled";
            else if (lso.status == 2)
              o.status = "Filled";
            else if (lso.status == 4)
              o.status = "Canceled";
            else if (lso.status == 8)
              o.status = "Rejected";

            auto it = orders_.insert(orders_.end(), o);
            orders_map_[lso.cl_ord_id] = it;
          }

          for (uint32_t i = 0; i < msg.execution_count; ++i) {
            const auto& lse = *reinterpret_cast<const LogonStateExecution*>(data + offset);
            offset += sizeof(lse);

            OdysseyExecutionData e;
            e.id = lse.cl_ord_id;
            e.id = "EXEC_HIST";
            e.symbol = lse.symbol;
            e.side = (lse.side == 1) ? "Buy" : "Sell";
            e.lastSz = lse.exec_qty;
            e.lastPx = lse.exec_px;
            e.cumQty = lse.exec_qty;
            e.avgPx = lse.exec_px;
            e.execType = "Historical Fill";
            executions_.push_back(e);
          }

          for (uint32_t i = 0; i < msg.reject_count; ++i) {
            const auto& lsr = *reinterpret_cast<const LogonStateReject*>(data + offset);
            offset += sizeof(lsr);

            OdysseyRejectData r;
            r.id = lsr.cl_ord_id;
            r.id = lsr.orig_cl_ord_id;
            r.refMsgType =
                lsr.is_cancel_replace_reject ? "CancelReplace" : "NewOrder";
            r.status = "Rejected";
            r.text = lsr.reason;
            rejections_.push_back(r);
          }

          rebuild_orders_grid();
          rebuild_executions_grid();
          rebuild_rejections_grid();
        break;
      }
      case MsgType::ExecReport: {
        const auto& msg = reinterpret_cast<const NetMsg<ExecReportMsg>*>(data)->payload;
        if (length >= sizeof(msg)) {

          std::string id = msg.cl_ord_id;
          std::string sym = msg.symbol;
          std::string side = (msg.side == 1) ? "Buy" : "Sell";

          auto it = orders_map_.find(id);
          if (it != orders_map_.end()) {
            auto &o = *it->second;
            o.cumQty = msg.cum_qty;
            o.leaves = msg.leaves_qty;
            o.avgPx = msg.cum_qty > 0 ? msg.last_px : 0.0;

            if (msg.status == 0)
              o.status = "New";
              else if (msg.status == 1)
                o.status = "Partially filled";
              else if (msg.status == 2)
                o.status = "Filled";
              else if (msg.status == 4)
                o.status = "Canceled";
              else if (msg.status == 5)
                o.status = "Replaced";
              else if (msg.status == 8)
                o.status = "Rejected";
          } else {
            OdysseyOrderData o;
            o.id = id;
            o.symbol = sym;
            o.side = side;
            o.price = msg.price;
            o.qty = msg.qty;
            o.cumQty = msg.cum_qty;
            o.leaves = msg.leaves_qty;
            o.avgPx = msg.last_px;

            if (msg.status == 0)
              o.status = "New";
            else if (msg.status == 1)
              o.status = "Partially filled";
            else if (msg.status == 2)
              o.status = "Filled";
            else if (msg.status == 4)
              o.status = "Canceled";
            else if (msg.status == 5)
              o.status = "Replaced";
            else if (msg.status == 8)
              o.status = "Rejected";

            auto it = orders_.insert(orders_.end(), o);
            orders_map_[msg.cl_ord_id] = it;

            if (auto prevIt = orders_map_.find(msg.orig_cl_ord_id); prevIt != orders_map_.end()) {
              orders_.erase(prevIt->second);
              orders_map_.erase(prevIt);
            }
          }

          if (msg.last_qty > 0) {
            OdysseyExecutionData exec;
            exec.id = id;
            exec.id = msg.order_id;
            exec.symbol = sym;
            exec.side = side;
            exec.lastSz = msg.last_qty;
            exec.lastPx = msg.last_px;
            exec.cumQty = msg.cum_qty;
            exec.avgPx = msg.last_px;
            exec.execType = (msg.status == 2) ? "Full Fill" : "Partial Fill";
            executions_.push_back(exec);

            rebuild_executions_grid();
          }

          rebuild_orders_grid();
        }
        break;
      }
      case MsgType::CancelReject: {
        const auto& msg = reinterpret_cast<const NetMsg<CancelRejectMsg>*>(data)->payload;
        if (length >= sizeof(msg)) {

          OdysseyRejectData r;
          r.id = msg.cl_ord_id;
          r.id = msg.orig_cl_ord_id;
          r.refMsgType = "Cancel / Replace";
          r.status = "Rejected";
          r.text = msg.reason;
          rejections_.push_back(r);

          rebuild_rejections_grid();
        }
        break;
      }
      case MsgType::MDUpdate: {
        const auto& msg = reinterpret_cast<const NetMsg<MDUpdateMsg>*>(data)->payload;
        if (length >= sizeof(msg)) {

          std::string sym = msg.symbol;

          bool found = false;
          for (auto &md : market_data_) {
            if (md.symbol == sym) {
              md.bidPx = msg.bid_px;
              md.bidSz = msg.bid_size;
              md.askPx = msg.ask_px;
              md.askSz = msg.ask_size;
              md.lastPx = msg.last_px;
              md.lastSz = msg.last_size;
              md.volume = msg.total_volume;
              md.is_invalid = false;
              
              if (md.row_ptr) {
                if (md.row_ptr->get_cell(1)) {
                  md.row_ptr->get_cell(1)->set_text_content_conflated(std::to_string(md.bidSz));
                  md.row_ptr->get_cell(2)->set_text_content_conflated(format_px(md.bidPx));
                  md.row_ptr->get_cell(3)->set_text_content_conflated(format_px(md.askPx));
                  md.row_ptr->get_cell(4)->set_text_content_conflated(std::to_string(md.askSz));
                  md.row_ptr->get_cell(5)->set_text_content_conflated(std::to_string(md.lastSz));
                  md.row_ptr->get_cell(6)->set_text_content_conflated(format_px(md.lastPx));
                  md.row_ptr->get_cell(7)->set_text_content_conflated(std::to_string(md.volume));
                }
                md.row_ptr->update_data(md);
              }
              
              found = true;
            }
          }

          if (!found) {
            OdysseyMarketData md;
            md.symbol = sym;
            md.bidPx = msg.bid_px;
            md.bidSz = msg.bid_size;
            md.askPx = msg.ask_px;
            md.askSz = msg.ask_size;
            md.lastPx = msg.last_px;
            md.lastSz = msg.last_size;
            md.volume = msg.total_volume;
            md.is_invalid = false;
            
            // Insert before the empty row if it exists
            bool inserted = false;
            for (auto it = market_data_.begin(); it != market_data_.end(); ++it) {
                if (it->symbol.empty()) {
                    market_data_.insert(it, md);
                    inserted = true;
                    break;
                }
            }
            if (!inserted) {
                market_data_.push_back(md);
            }
            rebuild_market_data_grid();
          }
        }
        break;
      }
      case MsgType::MDReject: {
        const auto& msg = reinterpret_cast<const NetMsg<MDRejectMsg>*>(data)->payload;
        if (length >= sizeof(msg)) {
            std::string sym = msg.symbol;
            bool found = false;
            for (auto &md : market_data_) {
                if (md.symbol == sym) {
                    md.is_invalid = true;
                    found = true;
                }
            }
            if (!found) {
                OdysseyMarketData md;
                md.symbol = sym;
                md.is_invalid = true;
                bool inserted = false;
                for (auto it = market_data_.begin(); it != market_data_.end(); ++it) {
                    if (it->symbol.empty()) {
                        market_data_.insert(it, md);
                        inserted = true;
                        break;
                    }
                }
                if (!inserted) market_data_.push_back(md);
            }
            rebuild_market_data_grid();
        }
        break;
      }
      case MsgType::Heartbeat: {
        std::cout << "Odyssey Client received Heartbeat from server." << std::endl;
        break;
      }
      default:
        break;
      }

      data += length;
      remaining_size -= length;
    }

    socketRecvBuf_.erase(socketRecvBuf_.begin(),
                         socketRecvBuf_.begin() + (size - remaining_size));
  }

  void updateStatusBar() {
    std::string state_text = "DISCONNECTED";
    std::string state_style =
        "display: inline-block; width: 10px; height: 10px; border-radius: 50%; "
        "margin-right: 8px; background-color: #f44336; box-shadow: 0 0 8px "
        "#f44336;";

    if (connected_state == 1) {
      state_text = "CONNECTED (LOGGING IN)";
      state_style = "display: inline-block; width: 10px; height: 10px; "
                    "border-radius: 50%; margin-right: 8px; background-color: "
                    "#ffb300; box-shadow: 0 0 8px #ffb300;";
    } else if (connected_state == 2) {
      state_text = "LOGGED ON";
      state_style = "display: inline-block; width: 10px; height: 10px; "
                    "border-radius: 50%; margin-right: 8px; background-color: "
                    "#4caf50; box-shadow: 0 0 8px #4caf50;";
    }

    if (order_dialog_submit_btn) {
      if (connected_state != 2) {
        order_dialog_submit_btn->set_attribute("disabled", "true");
        order_dialog_submit_btn->set_color("#aaa");
        order_dialog_submit_btn->set_text_color("#666");
      } else {
        order_dialog_submit_btn->set_property(ctx_->strings.disabled, false);
        order_dialog_submit_btn->set_color("#1a237e");
        order_dialog_submit_btn->set_text_color("white");
      }
    }

    status_light->set_attribute(ctx_->strings.style, state_style);

    std::stringstream ss;
    ss << "  Session: " << session_name_ << " (" << state_text << ")";
    status_text->set_text_content_conflated(ss.str());

    std::stringstream seq;
    seq << "Seq: " << sender_seq_num_ << ":" << target_seq_num_;
    seq_num_text->set_text_content_conflated(seq.str());

    std::stringstream sb_style;
    sb_style
        << "display: " << (status_bar_visible_ ? "flex" : "none")
        << "; justify-content: space-between; align-items: center; "
        << "background: rgba(255, 255, 255, 0.4); backdrop-filter: blur(10px); "
        << "color: #333; padding: 10px 20px; border-bottom: 1px solid "
           "rgba(255,255,255,0.4); "
        << "font-family: monospace; font-size: 13px; z-index: 1000; position: "
           "absolute; top: 0; left: 0; right: 0;";
    status_bar->set_attribute(ctx_->strings.style, sb_style.str());
  }

public:
  ~OdysseyTraderApp() {
    if (ws_ > 0) {
      emscripten_websocket_delete(ws_);
      ws_ = 0;
    }
  }

  void on_init(Context *ctx) override {
    ctx_ = ctx;
    g_odyssey_app_instance = this;

    // Base container
    main_div = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(main_div);
    Command cmd;
    cmd.type = CommandType::APPEND_CHILD;
    cmd.target_id = 0;
    cmd.arg1 = main_div->get_id();
    ctx_->send_command(cmd);

    // Styling with gorgeous pastel wallpaper pattern
    main_div->set_attribute(
        ctx->strings.style,
        "font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; "
        "background: linear-gradient(135deg, #fce4ec 0%, #e8eaf6 50%, #e0f2f1 "
        "100%); "
        "background-size: cover; width: 100vw; height: 100vh; position: "
        "absolute; "
        "left: 0; top: 0; overflow: hidden; margin: 0; padding: 0; box-sizing: "
        "border-box;");

    // Top Status Bar (Hidden until logon)
    status_bar = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(status_bar);
    main_div->append_child(status_bar);

    auto status_left = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(status_left);
    status_left->set_attribute(ctx->strings.style,
                               "display: flex; align-items: center;");
    status_bar->append_child(status_left);

    status_light = std::make_shared<Element>(ctx, ctx->register_string("span"));
    ctx->register_element(status_light);
    status_left->append_child(status_light);

    status_text = std::make_shared<Element>(ctx, ctx->register_string("span"));
    ctx->register_element(status_text);
    status_left->append_child(status_text);

    auto status_pref_btn = std::make_shared<Button>(ctx);
    ctx->register_element(status_pref_btn);
    status_pref_btn->set_text("Preferences");
    status_pref_btn->set_color("#5c6bc0");
    status_pref_btn->set_text_color("white");
    status_pref_btn->set_extra_style("margin-left: 20px; font-size: 12px; font-weight: bold; border-radius: 6px; box-shadow: 0 2px 4px rgba(0,0,0,0.2);");
    status_pref_btn->add_event_listener(ctx->strings.click, [this](const Event&) {
        pref_win_->set_visible(true);
        pref_win_->bring_to_front();
    });
    status_left->append_child(status_pref_btn);


    seq_num_text = std::make_shared<Element>(ctx, ctx->register_string("span"));
    ctx->register_element(seq_num_text);
    seq_num_text->set_attribute(ctx->strings.style,
                                "font-weight: bold; color: #1a237e;");
    status_bar->append_child(seq_num_text);

    updateStatusBar();

    // Initialize Windows
    init_windows();

    // Renders Logon Prompt
    render_logon_prompt();

    // Connect
    connectWebSocket();
  }

  void on_tick(double time) override {
    // Handle websocket status
    if (connected_state != last_connected_state) {
      updateStatusBar();
      last_connected_state = connected_state;
    }

    // Automatic Reconnect Loop (every 15 seconds)
    if (connected_state == 0 && ws_ == 0) { // DISCONNECTED and cleaned up
      if (last_connect_attempt_ == 0.0) {
        last_connect_attempt_ = time;
      }
      if (time - last_connect_attempt_ >= 15000.0) {
        std::cout << "Odyssey Client triggering automatic reconnect attempt..."
                  << std::endl;
        connectWebSocket();
      }
    } else if (connected_state == 1 && ws_ > 0) {
      if (last_logon_sent_time_ > 0.0 && time - last_logon_sent_time_ >= 5000.0) {
        std::cout << "Logon timeout. Reconnecting..." << std::endl;
        connectWebSocket();
      }
    }

    // Heartbeat logic — send every 30 seconds when logged on
    if (connected_state == 2 && ws_ > 0) {
      if (last_heartbeat_time_ == 0.0) {
        last_heartbeat_time_ = time;
      }
      if (time - last_heartbeat_time_ >= 30000.0) {
        std::cout << "Odyssey Client sending Heartbeat to server." << std::endl;
        MsgHeader msg;
        msg.type = htons(static_cast<uint16_t>(MsgType::Heartbeat));
        msg.length = htonl(sizeof(msg));
        send_binary_msg(&msg, sizeof(msg));
        last_heartbeat_time_ = time;
      }
    }
  }
};

OdysseyTraderApp *g_odyssey_app_instance = nullptr;
