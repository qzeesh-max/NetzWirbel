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
#include "NetzWirbel/DOM/Grid.hpp"
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
    std::shared_ptr<GridRow<MarketDataTickerRow>> row_ptr;
    int bid_size = 0;
    double bid_px = 0;
    double ask_px = 0;
    int ask_size = 0;
    double last_px = 0;
    long long total_vol = 0;
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

        g_grid = std::make_shared<Grid<MarketDataTickerRow>>(ctx);
        ctx->register_element(g_grid);
        g_grid->set_sort_enabled(false);
        g_grid->set_attribute(ctx_->strings.style, "flex: 1; overflow-y: auto; overflow-x: auto; background-color: #121212; color: #e0e0e0;");
        g_app->append_child(g_grid);

        auto style_el = std::make_shared<Element>(ctx, ctx->register_string("style"));
        ctx->register_element(style_el);
        style_el->set_text_content(R"(
            .md-cell {
                height: 35px;
                border-right: 1px solid #333;
                border-bottom: 1px solid #333;
                display: flex;
                align-items: center;
                color: #e0e0e0;
            }
            .md-row-even .md-cell { background-color: #1e1e1e; }
            .md-row-odd .md-cell { background-color: #252525; }
            .md-up { color: #00ff00 !important; }
            .md-down { color: #ff4444 !important; }
        )");
        g_app->append_child(style_el);

        g_grid->add_column("Symbol", 100);
        g_grid->add_column("Bid Size", 100);
        g_grid->add_column("Bid Price", 100);
        g_grid->add_column("Ask Price", 100);
        g_grid->add_column("Ask Size", 100);
        g_grid->add_column("Last Price", 100);
        g_grid->add_column("Total Vol", 150);

        g_grid->set_on_render_row([this](std::shared_ptr<GridRow<MarketDataTickerRow>> row, const MarketDataTickerRow& data) {
            row->add_cell(data.symbol, 100, "Symbol");
            row->add_cell(std::to_string(data.bid_size), 100, "Bid Size");
            row->add_cell(format_price(data.bid_px), 100, "Bid Price");
            row->add_cell(format_price(data.ask_px), 100, "Ask Price");
            row->add_cell(std::to_string(data.ask_size), 100, "Ask Size");
            row->add_cell(format_price(data.last_px), 100, "Last Price");
            row->add_cell(std::to_string(data.total_vol), 150, "Total Vol");

            std::string row_class = (row->seq_ % 2 == 0) ? "md-row-even" : "md-row-odd";
            row->set_attribute("class", row_class);
            int widths[] = {100, 100, 100, 100, 100, 100, 150};
            
            for (int i = 0; i < 7; ++i) {
                auto cell = row->get_cell(i);
                if (cell) {
                    std::stringstream ss;
                    ss << "padding: 6px; box-sizing: border-box; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; user-select: none; ";
                    ss << "width: " << widths[i] << "px; min-width: " << widths[i] << "px; max-width: " << widths[i] << "px;";
                    cell->set_attribute(ctx_->strings.style, ss.str());
                    cell->set_attribute("class", "md-cell");
                }
            }
        });

        for (int i = 0; i < 65; ++i) {
            MarketDataTickerRow tr;
            tr.symbol = "SYM" + std::to_string(i);
            tr.last_px = 0.0;
            tr.bid_px = 0.0;
            tr.ask_px = 0.0;
            tr.bid_size = 0;
            tr.ask_size = 0;
            tr.total_vol = 0;
            
            auto row_ptr = g_grid->add_row(tr);
            tr.row_ptr = row_ptr;
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
               << " | p99: " << format_price(stats.p99)
               << " | Conflated Hits: " << ctx_->get_conflation_hits();
            g_stats_pane->set_text_content_conflated(ss.str());
            last_stats_time = time;
        }
    }

private:
    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_app;
    std::shared_ptr<Grid<MarketDataTickerRow>> g_grid;
    std::shared_ptr<HTMLDivElement> g_stats_pane;
    std::shared_ptr<WebSocket> g_ws;
    std::vector<MarketDataTickerRow> g_tickers;

    std::string format_price(double p) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << p;
        return ss.str();
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
                            tr.total_vol = std::stoll(parts[6]);

                            auto row_ptr = tr.row_ptr;
                            if (row_ptr) {
                                row_ptr->get_cell(1)->set_text_content_conflated(parts[4]);
                                row_ptr->get_cell(2)->set_text_content_conflated(parts[2]);
                                row_ptr->get_cell(3)->set_text_content_conflated(parts[3]);
                                row_ptr->get_cell(4)->set_text_content_conflated(parts[5]);
                                row_ptr->get_cell(5)->set_text_content_conflated(parts[1]);
                                row_ptr->get_cell(6)->set_text_content_conflated(parts[6]);

                                std::string color_class = (tr.last_px >= old_last) ? "md-cell md-up" : "md-cell md-down";
                                row_ptr->get_cell(5)->set_class_conflated(color_class);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
};
