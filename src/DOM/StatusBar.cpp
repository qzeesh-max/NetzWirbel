/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "NetzWirbel/DOM/StatusBar.hpp"
#include "NetzWirbel/Context.hpp"

namespace NetzWirbel {

StatusBarPanel::StatusBarPanel(Context* ctx, BevelStyle style)
    : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, 
        "display: flex; align-items: center; padding: 0 8px; margin: 0 2px; "
        "height: calc(100% - 4px); font-size: 12px; color: #d0d0d0; "
        "white-space: nowrap; overflow: hidden; text-overflow: ellipsis; user-select: none;"
    );
    set_bevel_style(style);
}

void StatusBarPanel::set_bevel_style(BevelStyle style) {
    style_ = style;
    std::string border_style;
    switch (style) {
        case BevelStyle::Embossed:
            border_style = "border-top: 1px solid rgba(255,255,255,0.4); border-left: 1px solid rgba(255,255,255,0.4); "
                           "border-bottom: 1px solid rgba(0,0,0,0.6); border-right: 1px solid rgba(0,0,0,0.6);";
            break;
        case BevelStyle::Depressed:
            border_style = "border-top: 1px solid rgba(0,0,0,0.6); border-left: 1px solid rgba(0,0,0,0.6); "
                           "border-bottom: 1px solid rgba(255,255,255,0.4); border-right: 1px solid rgba(255,255,255,0.4);";
            break;
        case BevelStyle::Shaded:
            border_style = "background: linear-gradient(to bottom, rgba(0,0,0,0.1), rgba(0,0,0,0.3)); border-radius: 2px;";
            break;
        case BevelStyle::None:
        default:
            border_style = "border: 1px solid transparent;";
            break;
    }
    
    // Merge with existing style
    std::string current_style = "display: flex; align-items: center; padding: 0 8px; margin: 0 2px; height: calc(100% - 4px); font-size: 12px; color: #d0d0d0; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; user-select: none; ";
    set_attribute(ctx_->strings.style, current_style + border_style);
}

StatusBar::StatusBar(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, 
        "position: absolute; bottom: 0; left: 0; right: 0; height: 32px; "
        "background-color: rgba(30, 30, 30, 0.95); backdrop-filter: blur(10px); "
        "display: flex; align-items: center; border-top: 1px solid rgba(255, 255, 255, 0.1); "
        "z-index: 99998; box-sizing: border-box;"
    );
}

void StatusBar::add_panel(std::shared_ptr<StatusBarPanel> panel) {
    append_child(panel);
}

} // namespace NetzWirbel
