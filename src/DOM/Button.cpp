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

#include "NetzWirbel/DOM/Button.hpp"
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Event.hpp"
#include <sstream>

namespace NetzWirbel {

Button::Button(Context* ctx) : HTMLButtonElement(ctx) {
    icon_el_ = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
    ctx_->register_element(icon_el_);
    icon_el_->set_attribute(ctx_->strings.style, "display: none;");
    append_child(icon_el_);

    text_el_ = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
    ctx_->register_element(text_el_);
    text_el_->set_attribute(ctx_->strings.style, "display: none;");
    append_child(text_el_);

    apply_layout();

    add_event_listener(ctx_->register_string("mousedown"), [this](const Event& e) {
        set_active(true);
    });
    add_event_listener(ctx_->register_string("mouseup"), [this](const Event& e) {
        set_active(false);
    });
    add_event_listener(ctx_->register_string("mouseleave"), [this](const Event& e) {
        set_active(false);
    });
}

void Button::set_text(const std::string& text) {
    text_el_->set_text_content(ctx_->register_string(text));
    text_el_->set_attribute(ctx_->strings.style, "display: inline;");
}

void Button::set_icon(const std::string& icon_text) {
    icon_el_->set_text_content(ctx_->register_string(icon_text));
    icon_el_->set_attribute(ctx_->strings.style, "display: inline; font-size: 1.1em;");
}

void Button::set_alignment(Align align) {
    align_ = align;
    apply_layout();
}

void Button::set_color(const std::string& background_css) {
    bg_color_ = background_css;
    apply_layout();
}

void Button::set_text_color(const std::string& color_css) {
    text_color_ = color_css;
    apply_layout();
}

void Button::set_extra_style(const std::string& extra_style) {
    extra_style_ = extra_style;
    apply_layout();
}

void Button::apply_layout() {
    std::string justify = "center";
    if (align_ == Align::Left) justify = "flex-start";
    else if (align_ == Align::Right) justify = "flex-end";

    std::stringstream ss;
    ss << "display: flex; align-items: center; justify-content: " << justify << "; gap: 4px; "
       << "padding: 6px 12px; border-radius: 4px; cursor: pointer; "
       << "background: " << bg_color_ << "; "
       << "color: " << text_color_ << "; "
       << "border: 1px solid rgba(0,0,0,0.2); "
       << "font-family: inherit; font-size: 13px; font-weight: 500; "
       << "transition: transform 0.05s ease, box-shadow 0.05s ease; user-select: none; ";

    if (is_active_) {
        ss << "transform: scale(0.95); box-shadow: inset 0px 2px 4px rgba(0,0,0,0.2); ";
    } else {
        ss << "transform: scale(1.0); box-shadow: 0px 2px 4px rgba(0,0,0,0.1); ";
    }
    
    if (!extra_style_.empty()) {
        ss << extra_style_;
    }

    set_attribute(ctx_->strings.style, ss.str());
}

void Button::set_active(bool active) {
    if (get_disabled()) return;
    is_active_ = active;
    if (is_active_) {
        set_styles({
            {"transform", "scale(0.95)"},
            {"box-shadow", "inset 0px 2px 4px rgba(0,0,0,0.2)"}
        });
    } else {
        set_styles({
            {"transform", "scale(1.0)"},
            {"box-shadow", "0px 2px 4px rgba(0,0,0,0.1)"}
        });
    }
}

} // namespace NetzWirbel
