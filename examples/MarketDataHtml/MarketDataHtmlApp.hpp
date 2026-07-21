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

struct MarketDataHtmlTickerRow {
    std::string symbol;
    std::shared_ptr<HTMLDivElement> bid_sz_el;
    std::shared_ptr<HTMLDivElement> bid_px_el;
    std::shared_ptr<HTMLDivElement> ask_px_el;
    std::shared_ptr<HTMLDivElement> ask_sz_el;
    std::shared_ptr<HTMLDivElement> last_px_el;
    std::shared_ptr<HTMLDivElement> vol_el;

    double last_px = 0;
};

class MarketDataHtmlApp : public App {
public:
    void on_init(Context* ctx) override {
        this->ctx_ = ctx;
        
        for (int i = 0; i < 65; ++i) {
            MarketDataHtmlTickerRow tr;
            tr.symbol = "SYM" + std::to_string(i);
            tr.last_px = 0.0;
            
            tr.bid_sz_el = std::make_shared<HTMLDivElement>(ctx);
            tr.bid_px_el = std::make_shared<HTMLDivElement>(ctx);
            tr.ask_px_el = std::make_shared<HTMLDivElement>(ctx);
            tr.ask_sz_el = std::make_shared<HTMLDivElement>(ctx);
            tr.last_px_el = std::make_shared<HTMLDivElement>(ctx);
            tr.vol_el = std::make_shared<HTMLDivElement>(ctx);

            ctx->register_element(tr.bid_sz_el);
            ctx->register_element(tr.bid_px_el);
            ctx->register_element(tr.ask_px_el);
            ctx->register_element(tr.ask_sz_el);
            ctx->register_element(tr.last_px_el);
            ctx->register_element(tr.vol_el);

            ctx->bind_element(tr.bid_sz_el, "#bid-sz-" + tr.symbol);
            ctx->bind_element(tr.bid_px_el, "#bid-px-" + tr.symbol);
            ctx->bind_element(tr.ask_px_el, "#ask-px-" + tr.symbol);
            ctx->bind_element(tr.ask_sz_el, "#ask-sz-" + tr.symbol);
            ctx->bind_element(tr.last_px_el, "#last-px-" + tr.symbol);
            ctx->bind_element(tr.vol_el, "#vol-" + tr.symbol);
            
            g_tickers.push_back(tr);
        }

        g_ws = std::make_shared<WebSocket>(ctx, "ws://localhost:3000");
        g_ws->on_message([this](const std::string& msg) {
            this->on_market_data(msg);
        });

        g_stats_pane = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_stats_pane);
        ctx->bind_element(g_stats_pane, "#stats");
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
            g_stats_pane->set_text_content_conflated(ss.str());
            last_stats_time = time;
        }
    }

private:
    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_stats_pane;
    std::shared_ptr<WebSocket> g_ws;
    std::vector<MarketDataHtmlTickerRow> g_tickers;

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

                            tr.bid_sz_el->set_text_content_conflated(parts[4]);
                            tr.bid_px_el->set_text_content_conflated(parts[2]);
                            tr.ask_px_el->set_text_content_conflated(parts[3]);
                            tr.ask_sz_el->set_text_content_conflated(parts[5]);
                            tr.last_px_el->set_text_content_conflated(parts[1]);
                            tr.vol_el->set_text_content_conflated(parts[6]);

                            std::string color = (tr.last_px >= old_last) ? "#00ff00" : "#ff4444";
                            tr.last_px_el->set_style("color", color);
                            break;
                        }
                    }
                }
            }
        }
    }
};


