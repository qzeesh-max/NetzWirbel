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
#include <functional>
#include <memory>

namespace NetzWirbel {

class Button;

class Spinner : public HTMLDivElement {
public:
    Spinner(Context* ctx, double initial_value = 0.0, double step = 1.0, int precision = 2);
    virtual ~Spinner() = default;

    void set_value(double val);
    double get_value() const { return value_; }

    void set_step(double step) { step_ = step; }
    void set_precision(int precision) { precision_ = precision; update_display(); }

    void set_on_change(std::function<void(double)> cb) { on_change_ = cb; }

private:
    void update_display();
    void end_edit();

    double value_;
    double step_;
    int precision_;
    
    std::shared_ptr<Button> btn_minus_;
    std::shared_ptr<Button> btn_plus_;
    std::shared_ptr<HTMLDivElement> display_container_;
    std::shared_ptr<Element> display_text_;
    std::shared_ptr<HTMLInputElement> edit_input_;

    std::function<void(double)> on_change_;
    bool is_editing_ = false;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
