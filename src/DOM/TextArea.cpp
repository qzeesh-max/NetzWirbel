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

#include "NetzWirbel/DOM/TextArea.hpp"
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Event.hpp"

namespace NetzWirbel {

TextArea::TextArea(Context* ctx) : Element(ctx, ctx->strings.textarea) {
    set_attribute(ctx_->strings.style, "padding: 6px 10px; border: 1px solid #ccc; border-radius: 4px; outline: none; font-family: inherit; font-size: 13px; resize: vertical; min-height: 60px; user-select: text;");

    add_event_listener(ctx_->register_string("input"), [this](const Event& e) {
        if (on_change_) {
            on_change_(get_text());
        }
    });
}

void TextArea::set_text(const std::string& text) {
    text_ = text;
    set_text_content(ctx_->register_string(text));
}

std::string TextArea::get_text() const {
    return text_;
}

void TextArea::set_placeholder(const std::string& placeholder) {
    set_attribute(ctx_->strings.placeholder, placeholder);
}

} // namespace NetzWirbel
