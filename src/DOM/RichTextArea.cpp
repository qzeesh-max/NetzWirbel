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

#include "NetzWirbel/DOM/RichTextArea.hpp"
#include "NetzWirbel/Context.hpp"

namespace NetzWirbel {

RichTextArea::RichTextArea(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->register_string("contenteditable"), "true");
    set_attribute(ctx_->strings.style, "padding: 8px; border: 1px solid #ccc; border-radius: 4px; outline: none; min-height: 80px; overflow-y: auto; background: white; user-select: text;");
}

void RichTextArea::set_html(const std::string& html) {
    // Basic setting of inner HTML via command to JS is ideally needed
    // Assuming set_text_content acts as innerHTML in NetzWirbel for now, or just setting raw text
    set_text_content(html);
}

} // namespace NetzWirbel
