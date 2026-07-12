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

#include "NetzWirbel/App.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include "NetzWirbel/Network.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

using namespace NetzWirbel;

struct MarketDataTickerRow {
    std::string symbol;
    std::shared_ptr<HTMLDivElement> cells[7];
    int bid_size = 0;
    double bid_px = 0;
    double ask_px = 0;
    int ask_size = 0;
    double last_px = 0;
    int total_vol = 0;
};

class MarketDataApp : public App {
public:
    void on_init(Context* ctx) override {
        this->ctx_ = ctx;
        
        g_app = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_app);
        g_app->set_attribute(ctx_->strings.style, "display: flex; flex-direction: column; height: 100vh; background-color: #121212; color: #e0e0e0; font-family: monospace; margin: -8px;");
        
        Command cmd;
        cmd.type = CommandType::APPEND_CHILD;
        cmd.target_id = 0;
        cmd.arg1 = g_app->get_id();
        ctx->send_command(cmd);

        g_grid_container = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_grid_container);
        g_grid_container->set_attribute(ctx_->strings.style, "flex: 1; overflow-y: auto; overflow-x: auto; display: grid; grid-template-columns: 1fr 1fr 1fr 1fr 1fr 1fr 1.5fr; align-content: start;");
        g_app->append_child(g_grid_container);

        const char* headers[] = {"Symbol", "Bid Size", "Bid Price", "Ask Price", "Ask Size", "Last Price", "Total Vol"};
        for (int i = 0; i < 7; ++i) {
            auto th = create_cell(ctx, headers[i], true);
            ctx->register_element(th);
            g_grid_container->append_child(th);
        }

        for (int i = 0; i < 65; ++i) {
            MarketDataTickerRow tr;
            tr.symbol = "SYM" + std::to_string(i);
            tr.last_px = 0.0;
            tr.bid_px = 0.0;
            tr.ask_px = 0.0;
            tr.bid_size = 0;
            tr.ask_size = 0;
            tr.total_vol = 0;
            
            std::string row_bg = (i % 2 == 0) ? "#1e1e1e" : "#252525";
            std::string base_style = "padding: 0 8px; border-right: 1px solid #333; border-bottom: 1px solid #333; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; box-sizing: border-box; height: 35px; display: flex; align-items: center; background-color: " + row_bg + ";";

            for (int c = 0; c < 7; ++c) {
                tr.cells[c] = create_cell(ctx, "");
                tr.cells[c]->set_attribute(ctx_->strings.style, base_style);
                ctx->register_element(tr.cells[c]);
                g_grid_container->append_child(tr.cells[c]);
            }
            tr.cells[0]->set_text_content(tr.symbol);
            
            g_tickers.push_back(tr);
        }

        g_ws = std::make_shared<WebSocket>(ctx, "ws://localhost:3000");
        g_ws->on_message([this](const std::string& msg) {
            this->on_market_data(msg);
        });

        g_stats_pane = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_stats_pane);
        g_stats_pane->set_attribute(ctx_->strings.style, "height: 40px; background-color: #000; border-top: 2px solid #00e5ff; padding: 10px; overflow-y: auto; display: flex; align-items: center; white-space: pre-wrap; font-size: 14px;");
        g_stats_pane->set_text_content("Performance Stats: initializing...");
        g_app->append_child(g_stats_pane);
    }

    void on_tick(double time) override {
        ctx_->send_ping();

        static double last_stats_time = 0;
        if (time - last_stats_time > 1000) {
            auto stats = ctx_->get_rtt_stats();
            std::stringstream ss;
            ss << "RTT (ms) | Low: " << format_price(stats.low)
               << " | High: " << format_price(stats.high)
               << " | Avg: " << format_price(stats.avg)
               << " | Median: " << format_price(stats.median)
               << " | Mode: " << format_price(stats.mode)
               << " | StdDev: " << format_price(stats.stddev)
               << " | p99: " << format_price(stats.p99);
            g_stats_pane->set_text_content(ss.str());
            last_stats_time = time;
        }
    }

private:
    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_app;
    std::shared_ptr<HTMLDivElement> g_grid_container;
    std::shared_ptr<HTMLDivElement> g_stats_pane;
    std::shared_ptr<WebSocket> g_ws;
    std::vector<MarketDataTickerRow> g_tickers;

    std::string format_price(double p) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << p;
        return ss.str();
    }

    std::shared_ptr<HTMLDivElement> create_cell(Context* ctx, const std::string& text, bool is_header = false) {
        auto el = std::make_shared<HTMLDivElement>(ctx);
        el->set_text_content(text);
        std::string base_style = "padding: 0 8px; border-right: 1px solid #333; border-bottom: 1px solid #333; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; box-sizing: border-box; height: 35px; display: flex; align-items: center;";
        if (is_header) {
            base_style += " background-color: #1a1a1a; font-weight: bold; resize: horizontal; border-bottom: 2px solid #444; position: sticky; top: 0; z-index: 10;";
        }
        el->set_attribute(ctx_->strings.style, base_style);
        return el;
    }

    void on_market_data(const std::string& msg) {
        std::istringstream stream(msg);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.substr(0, 3) == "MD|") {
                std::vector<std::string> parts;
                std::istringstream ls(line.substr(3));
                std::string part;
                while (std::getline(ls, part, '|')) {
                    parts.push_back(part);
                }
                if (parts.size() == 7) {
                    std::string symbol = parts[0];
                    for (size_t i = 0; i < g_tickers.size(); ++i) {
                        auto& tr = g_tickers[i];
                        if (tr.symbol == symbol) {
                            double old_last = tr.last_px;
                            
                            tr.last_px = std::stod(parts[1]);
                            tr.bid_px = std::stod(parts[2]);
                            tr.ask_px = std::stod(parts[3]);
                            tr.bid_size = std::stoi(parts[4]);
                            tr.ask_size = std::stoi(parts[5]);
                            tr.total_vol = std::stoi(parts[6]);

                            tr.cells[1]->set_text_content(parts[4]);
                            tr.cells[2]->set_text_content(parts[2]);
                            tr.cells[3]->set_text_content(parts[3]);
                            tr.cells[4]->set_text_content(parts[5]);
                            tr.cells[5]->set_text_content(parts[1]);
                            tr.cells[6]->set_text_content(parts[6]);

                            std::string row_bg = (i % 2 == 0) ? "#1e1e1e" : "#252525";
                            std::string color = (tr.last_px >= old_last) ? "#00ff00" : "#ff4444";
                            std::string cell_style = "padding: 0 8px; border-right: 1px solid #333; border-bottom: 1px solid #333; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; box-sizing: border-box; height: 35px; display: flex; align-items: center; background-color: " + row_bg + "; color: " + color + "; transition: color 0.2s;";
                            
                            tr.cells[5]->set_attribute(ctx_->strings.style, cell_style);
                            break;
                        }
                    }
                }
            }
        }
    }
};
