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
#include "NetzWirbel/DOM/Elements.hpp"
#include <string>

namespace NetzWirbel {

enum class Align {
    Left, Center, Right
};

class Button : public HTMLButtonElement {
public:
    Button(Context* ctx);
    virtual ~Button() = default;

    void set_text(const std::string& text);
    void set_icon(const std::string& icon_text);
    
    void set_alignment(Align align);
    void set_color(const std::string& background_css);
    void set_text_color(const std::string& color_css);
    void set_extra_style(const std::string& extra_style);
    
    // Trigger visual click effect (useful for programmatic clicks)
    void set_active(bool active);

private:
    void apply_layout();

    std::shared_ptr<Element> icon_el_;
    std::shared_ptr<Element> text_el_;
    
    Align align_ = Align::Center;
    std::string bg_color_ = "linear-gradient(to bottom, #f0f0f0, #e0e0e0)";
    std::string text_color_ = "#333";
    std::string extra_style_ = "";
    bool is_active_ = false;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
