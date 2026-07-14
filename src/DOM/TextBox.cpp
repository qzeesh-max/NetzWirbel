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

#include "NetzWirbel/DOM/TextBox.hpp"
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Event.hpp"

namespace NetzWirbel {

TextBox::TextBox(Context* ctx) : HTMLInputElement(ctx) {
    set_attribute(ctx_->strings.type, "text");
    set_attribute(ctx_->strings.style, "padding: 6px 10px; border: 1px solid #ccc; border-radius: 4px; outline: none; font-family: inherit; font-size: 13px; user-select: text;");

    add_event_listener(ctx_->register_string("input"), [this](const Event& e) {
        if (on_change_) {
            on_change_(get_text());
        }
    });
}

void TextBox::set_text(const std::string& text) {
    set_attribute(ctx_->strings.value, text);
}

std::string TextBox::get_text() const {
    return "";
}

void TextBox::set_placeholder(const std::string& placeholder) {
    set_attribute(ctx_->strings.placeholder, placeholder);
}

} // namespace NetzWirbel
