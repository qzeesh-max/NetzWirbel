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

#include "NetzWirbel/DOM/Spinner.hpp"
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Button.hpp"
#include "NetzWirbel/DOM/Event.hpp"
#include <sstream>
#include <iomanip>

namespace NetzWirbel {

Spinner::Spinner(Context* ctx, double initial_value, double step, int precision)
    : HTMLDivElement(ctx), value_(initial_value), step_(step), precision_(precision) {
    
    set_attribute(ctx_->strings.style, "display: inline-flex; align-items: center; border: 1px solid #ccc; border-radius: 4px; overflow: hidden; background: white;");

    btn_minus_ = std::make_shared<Button>(ctx_);
    ctx_->register_element(btn_minus_);
    btn_minus_->set_icon("-");
    btn_minus_->set_attribute(ctx_->strings.style, "border: none; border-radius: 0; border-right: 1px solid #ccc; padding: 4px 8px; cursor: pointer; background: #f9f9f9;");
    append_child(btn_minus_);

    display_container_ = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(display_container_);
    display_container_->set_attribute(ctx_->strings.style, "padding: 4px 12px; min-width: 50px; text-align: center; cursor: pointer; position: relative; display: flex; align-items: center; justify-content: center;");
    append_child(display_container_);

    display_text_ = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
    ctx_->register_element(display_text_);
    display_container_->append_child(display_text_);

    edit_input_ = std::make_shared<HTMLInputElement>(ctx_);
    ctx_->register_element(edit_input_);
    edit_input_->set_attribute(ctx_->strings.type, "number");
    edit_input_->set_attribute(ctx_->strings.style, "display: none; position: absolute; left: 0; top: 0; width: 100%; height: 100%; box-sizing: border-box; text-align: center; border: none; outline: none;");
    display_container_->append_child(edit_input_);

    btn_plus_ = std::make_shared<Button>(ctx_);
    ctx_->register_element(btn_plus_);
    btn_plus_->set_icon("+");
    btn_plus_->set_attribute(ctx_->strings.style, "border: none; border-radius: 0; border-left: 1px solid #ccc; padding: 4px 8px; cursor: pointer; background: #f9f9f9;");
    append_child(btn_plus_);

    update_display();

    btn_minus_->add_event_listener(ctx_->strings.click, [this](const Event& e) {
        set_value(value_ - step_);
        if (on_change_) on_change_(value_);
    });

    btn_plus_->add_event_listener(ctx_->strings.click, [this](const Event& e) {
        set_value(value_ + step_);
        if (on_change_) on_change_(value_);
    });

    display_container_->add_event_listener(ctx_->register_string("dblclick"), [this](const Event& e) {
        is_editing_ = true;
        display_text_->set_attribute(ctx_->strings.style, "display: none;");
        edit_input_->set_attribute(ctx_->strings.style, "display: block; position: absolute; left: 0; top: 0; width: 100%; height: 100%; box-sizing: border-box; text-align: center; border: none; outline: none;");
        std::stringstream ss;
        if (precision_ == 0) ss << (long long)value_;
        else ss << std::fixed << std::setprecision(precision_) << value_;
        edit_input_->set_attribute(ctx_->strings.value, ss.str());
    });

    edit_input_->add_event_listener(ctx_->register_string("blur"), [this](const Event& e) {
        end_edit();
    });
}

void Spinner::end_edit() {
    if (!is_editing_) return;
    is_editing_ = false;
    display_text_->set_attribute(ctx_->strings.style, "display: block;");
    edit_input_->set_attribute(ctx_->strings.style, "display: none;");
    try {
        // We might not easily get value unless updated via Property change messages
        // If getting value isn't supported, we fallback to no-op
    } catch (...) {}
    update_display();
}

void Spinner::set_value(double val) {
    value_ = val;
    update_display();
}

void Spinner::update_display() {
    std::stringstream ss;
    if (precision_ == 0) {
        ss << (long long)value_;
    } else {
        ss << std::fixed << std::setprecision(precision_) << value_;
    }
    display_text_->set_text_content(ctx_->register_string(ss.str()));
}

} // namespace NetzWirbel
