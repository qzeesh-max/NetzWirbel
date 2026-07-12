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
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iomanip>

using namespace NetzWirbel;

class CardGameApp : public App {
public:
    void on_init(Context* ctx) override {
        this->ctx_ = ctx;
        srand(1234);
        
        g_app = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_app);
        g_app->set_attribute(ctx_->strings.style, "display: flex; flex-direction: column; height: 100vh; background-color: #f0f0f0; margin: -8px; font-family: sans-serif;");
        
        Command cmd;
        cmd.type = CommandType::APPEND_CHILD;
        cmd.target_id = 0;
        cmd.arg1 = g_app->get_id();
        ctx->send_command(cmd);

        g_container = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_container);
        g_container->set_attribute(ctx_->strings.style, "flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center;");
        g_app->append_child(g_container);

        auto title = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(title);
        title->set_text_content("High-Low Card Game");
        title->set_attribute(ctx_->strings.style, "font-size: 2.5em; font-weight: bold; margin-bottom: 20px; color: #333;");
        g_container->append_child(title);

        g_score_display = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_score_display);
        g_score_display->set_attribute(ctx_->strings.style, "font-size: 1.5em; margin-bottom: 15px;");
        g_container->append_child(g_score_display);

        g_message_display = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_message_display);
        g_message_display->set_text_content("Guess if the next card is higher or lower.");
        g_message_display->set_attribute(ctx_->strings.style, "color: #555; margin-bottom: 20px; font-size: 1.2em;");
        g_container->append_child(g_message_display);

        // HTMLImageElement
        g_card_image = std::make_shared<HTMLImageElement>(ctx);
        ctx->register_element(g_card_image);
        g_card_image->set_attribute(ctx_->strings.style, "width: 150px; height: 210px; margin-bottom: 30px; border-radius: 10px; box-shadow: 2px 5px 15px rgba(0,0,0,0.3);");
        g_container->append_child(g_card_image);

        auto buttons_container = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(buttons_container);
        buttons_container->set_attribute(ctx_->strings.style, "display: flex; gap: 20px;");
        g_container->append_child(buttons_container);

        auto btn_higher = std::make_shared<HTMLButtonElement>(ctx);
        ctx->register_element(btn_higher);
        btn_higher->set_text_content("Higher");
        btn_higher->set_attribute(ctx_->strings.style, "padding: 15px 30px; font-size: 1.2em; cursor: pointer; background-color: #4CAF50; color: white; border: none; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);");
        btn_higher->add_event_listener(ctx_->strings.click, [this](const Event& e) {
            this->handle_guess(true);
        });
        buttons_container->append_child(btn_higher);

        auto btn_lower = std::make_shared<HTMLButtonElement>(ctx);
        ctx->register_element(btn_lower);
        btn_lower->set_text_content("Lower");
        btn_lower->set_attribute(ctx_->strings.style, "padding: 15px 30px; font-size: 1.2em; cursor: pointer; background-color: #f44336; color: white; border: none; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);");
        btn_lower->add_event_listener(ctx_->strings.click, [this](const Event& e) {
            this->handle_guess(false);
        });
        buttons_container->append_child(btn_lower);

        g_stats_pane = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_stats_pane);
        g_stats_pane->set_attribute(ctx_->strings.style, "height: 40px; background-color: #333; color: #fff; padding: 10px; display: flex; align-items: center; white-space: pre-wrap; font-size: 14px; font-family: monospace;");
        g_stats_pane->set_text_content("Performance Stats: initializing...");
        g_app->append_child(g_stats_pane);

        draw_new_card();
        update_score();
    }

    void on_tick(double time) override {
        ctx_->send_ping();
        
        static double last_stats_time = 0;
        if (time - last_stats_time > 1000) {
            auto stats = ctx_->get_rtt_stats();
            std::stringstream ss;
            ss << "RTT (ms) | Low: " << format_number(stats.low)
               << " | High: " << format_number(stats.high)
               << " | Avg: " << format_number(stats.avg)
               << " | Median: " << format_number(stats.median)
               << " | Mode: " << format_number(stats.mode)
               << " | StdDev: " << format_number(stats.stddev)
               << " | p99: " << format_number(stats.p99);
            g_stats_pane->set_text_content(ss.str());
            last_stats_time = time;
        }
    }

private:
    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_app;
    std::shared_ptr<HTMLDivElement> g_container;
    std::shared_ptr<HTMLImageElement> g_card_image;
    std::shared_ptr<HTMLDivElement> g_score_display;
    std::shared_ptr<HTMLDivElement> g_message_display;
    std::shared_ptr<HTMLDivElement> g_stats_pane;

    int current_value = 0;
    int current_suit = 0;
    int score = 0;

    std::string get_card_name(int v, int s) {
        const char* VALS[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "0", "J", "Q", "K"};
        std::string val = VALS[v];
        if (val == "0") val = "10";
        std::string suit_name;
        if (s == 0) suit_name = "Spades";
        if (s == 1) suit_name = "Hearts";
        if (s == 2) suit_name = "Diamonds";
        if (s == 3) suit_name = "Clubs";
        return val + " of " + suit_name;
    }

    void draw_new_card() {
        current_value = rand() % 13;
        current_suit = rand() % 4;
        
        const char* VALS[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "0", "J", "Q", "K"};
        const char* SUITS[] = {"S", "H", "D", "C"};
        std::string code = std::string(VALS[current_value]) + std::string(SUITS[current_suit]);
        std::string url = "/proxy-image?url=https://deckofcardsapi.com/static/img/" + code + ".png";
        
        if (g_card_image) {
            g_card_image->set_src(url);
        }
    }

    void update_score() {
        if (g_score_display) {
            g_score_display->set_text_content("Score: " + std::to_string(score));
        }
    }

    void handle_guess(bool higher) {
        int next_value = rand() % 13;
        int next_suit = rand() % 4;
        
        std::string msg = "Next card was " + get_card_name(next_value, next_suit) + ". ";
        
        if ((higher && next_value >= current_value) || (!higher && next_value <= current_value)) {
            score++;
            msg += "You guessed correctly!";
            g_message_display->set_attribute(ctx_->strings.style, "color: green; font-weight: bold; margin-bottom: 20px; font-size: 1.2em;");
        } else {
            score = 0;
            msg += "You guessed wrong! Score reset.";
            g_message_display->set_attribute(ctx_->strings.style, "color: red; font-weight: bold; margin-bottom: 20px; font-size: 1.2em;");
        }
        
        current_value = next_value;
        current_suit = next_suit;
        
        const char* VALS[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "0", "J", "Q", "K"};
        const char* SUITS[] = {"S", "H", "D", "C"};
        std::string code = std::string(VALS[current_value]) + std::string(SUITS[current_suit]);
        std::string url = "/proxy-image?url=https://deckofcardsapi.com/static/img/" + code + ".png";
        g_card_image->set_src(url);
        
        g_message_display->set_text_content(msg);
        update_score();
    }

    std::string format_number(double p) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << p;
        return ss.str();
    }
};


