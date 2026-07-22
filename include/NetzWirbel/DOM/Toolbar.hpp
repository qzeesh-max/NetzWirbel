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
#include <vector>
#include <memory>
#include <functional>

namespace NetzWirbel {

struct ToolbarButtonConfig {
    std::string icon;
    std::string tooltip;
    std::function<void()> on_click;
};

struct ToolbarConfig {
    std::vector<ToolbarButtonConfig> buttons;
    bool is_vertical = false;
    std::string bg_color = "rgba(40, 40, 40, 0.9)";
};

class ToolbarButton : public HTMLButtonElement {
public:
    ToolbarButton(Context* ctx, const std::string& icon, const std::string& tooltip);
};

class Toolbar : public HTMLDivElement {
public:
    Toolbar(Context* ctx);
    
    static std::shared_ptr<Toolbar> createFromTemplate(Context* ctx, const ToolbarConfig& config);

    void add_button(std::shared_ptr<ToolbarButton> button);
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
