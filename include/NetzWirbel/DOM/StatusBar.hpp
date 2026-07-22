/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once
#include "NetzWirbel/DOM/Element.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include <string>
#include <memory>

namespace NetzWirbel {

enum class BevelStyle {
    None,
    Embossed,
    Depressed,
    Shaded
};

class StatusBarPanel : public HTMLDivElement {
public:
    StatusBarPanel(Context* ctx, BevelStyle style = BevelStyle::None);
    void set_bevel_style(BevelStyle style);
private:
    BevelStyle style_;
};

class StatusBar : public HTMLDivElement {
public:
    StatusBar(Context* ctx);
    void add_panel(std::shared_ptr<StatusBarPanel> panel);
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
