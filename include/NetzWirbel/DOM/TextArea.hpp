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

#pragma once
#include "NetzWirbel/DOM/Element.hpp"
#include <string>
#include <functional>

namespace NetzWirbel {

class TextArea : public Element {
public:
    TextArea(Context* ctx);
    virtual ~TextArea() = default;

    void set_text(const std::string& text);
    std::string get_text() const;

    void set_placeholder(const std::string& placeholder);

    void set_on_change(std::function<void(const std::string&)> cb) { on_change_ = cb; }

private:
    std::function<void(const std::string&)> on_change_;
    std::string text_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
