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

using namespace NetzWirbel;

class CalculatorApp : public App {
public:
    void on_init(Context* ctx) override {
        ctx_ = ctx;
        std::cout << "NetzWirbel Calculator Initialized!" << std::endl;

        g_container = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(g_container);
        g_container->set_attribute(ctx->strings.id, "calculator-container");
        g_container->set_attribute(ctx->strings.style, "display: flex; flex-direction: column; width: 200px; padding: 10px; border: 1px solid #ccc; border-radius: 5px; font-family: sans-serif;");
        
        // Append container to document.body (targetId = 0)
        Command cmd;
        cmd.type = CommandType::APPEND_CHILD;
        cmd.target_id = 0; // body
        cmd.arg1 = g_container->get_id();
        ctx->send_command(cmd);

        g_display = std::make_shared<HTMLInputElement>(ctx);
        ctx->register_element(g_display);
        g_display->set_attribute(ctx->strings.type, ctx->strings.text);
        g_display->set_attribute(ctx_->strings.value, ctx->strings.zero);
        g_display->set_attribute(ctx->register_string("readonly"), ctx->strings.true_);
        g_display->set_attribute(ctx->strings.style, "text-align: right; margin-bottom: 10px; padding: 5px; font-size: 1.2em;");
        g_container->append_child(g_display);

        auto grid = std::make_shared<HTMLDivElement>(ctx);
        ctx->register_element(grid);
        grid->set_attribute(ctx->strings.style, "display: grid; grid-template-columns: repeat(4, 1fr); gap: 5px;");
        g_container->append_child(grid);

        std::vector<std::string> buttons = {
            "7", "8", "9", "/",
            "4", "5", "6", "*",
            "1", "2", "3", "-",
            "C", "0", ".", "+"
        };
        buttons.push_back("=");

        for (const auto& label : buttons) {
            auto btn = std::make_shared<HTMLButtonElement>(ctx);
            ctx->register_element(btn);
            btn->set_text_content(label);
            if (label == "=") {
                btn->set_attribute(ctx->strings.style, "grid-column: span 4; padding: 10px; font-size: 1.1em; cursor: pointer;");
            } else {
                btn->set_attribute(ctx->strings.style, "padding: 10px; font-size: 1.1em; cursor: pointer;");
            }
            
            btn->add_event_listener(ctx->strings.click, [this, label](const Event& e) {
                this->on_button_click(label);
            });
            
            grid->append_child(btn);
        }

        g_window = std::make_shared<WindowElement>(ctx);
        ctx->register_element(g_window);
        ctx->bind_element(g_window, "window");
        g_window->add_event_listener(ctx->strings.keydown, [this](const Event& e) {
            auto ke = dynamic_cast<const KeyboardEvent*>(&e);
            if (ke) {
                std::string key = ke->get_key();
                std::string mapped = "";
                if (key >= "0" && key <= "9") mapped = key;
                else if (key == "+" || key == "-" || key == "*" || key == "/" || key == ".") mapped = key;
                else if (key == "Enter" || key == "=") mapped = "=";
                else if (key == "Escape" || key == "Backspace" || key == "Delete") mapped = "C";

                if (!mapped.empty()) {
                    this->on_button_click(mapped);
                }
            }
        }, true);
    }

private:
    Context* ctx_ = nullptr;
    std::shared_ptr<HTMLDivElement> g_container;
    std::shared_ptr<HTMLInputElement> g_display;
    std::shared_ptr<WindowElement> g_window;

    std::string current_input = "0";
    std::string previous_input = "";
    std::string current_op = "";
    bool new_input_needed = false;

    void update_display() {
        if (g_display) {
            g_display->set_attribute(ctx_->strings.value, current_input);
        }
    }

    void on_button_click(const std::string& label) {
        if (current_input == "Error" && label != "C") {
            current_input = "0";
            previous_input = "";
            current_op = "";
            new_input_needed = false;
        }

        if (label >= "0" && label <= "9") {
            if (current_input == "0" || new_input_needed) {
                current_input = label;
                new_input_needed = false;
            } else {
                current_input += label;
            }
        } else if (label == ".") {
            if (new_input_needed) {
                current_input = "0.";
                new_input_needed = false;
            } else if (current_input.find(".") == std::string::npos) {
                current_input += ".";
            }
        } else if (label == "C") {
            current_input = "0";
            previous_input = "";
            current_op = "";
            new_input_needed = false;
        } else if (label == "=") {
            if (!current_op.empty() && !previous_input.empty()) {
                double prev = std::stod(previous_input);
                double curr = std::stod(current_input);
                double result = 0;
                if (current_op == "+") result = prev + curr;
                if (current_op == "-") result = prev - curr;
                if (current_op == "*") result = prev * curr;
                if (current_op == "/") {
                    if (curr == 0) {
                        current_input = "Error";
                        current_op = "";
                        previous_input = "";
                        new_input_needed = true;
                        update_display();
                        return;
                    }
                    result = prev / curr;
                }
                
                current_input = std::to_string(result);
                // Remove trailing zeros
                current_input.erase(current_input.find_last_not_of('0') + 1, std::string::npos);
                if (current_input.back() == '.') current_input.pop_back();
                
                current_op = "";
                previous_input = "";
                new_input_needed = true;
            }
        } else {
            // Operator
            if (!current_op.empty() && !new_input_needed) {
                // chaining operators
                on_button_click("=");
                if (current_input == "Error") return;
            }
            previous_input = current_input;
            current_op = label;
            new_input_needed = true;
        }
        update_display();
    }
};


