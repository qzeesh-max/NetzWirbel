/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "NetzWirbel/DOM/Toolbar.hpp"
#include "NetzWirbel/Context.hpp"

namespace NetzWirbel {

ToolbarButton::ToolbarButton(Context* ctx, const std::string& icon, const std::string& tooltip)
    : HTMLButtonElement(ctx) {
    
    set_attribute(ctx_->strings.style, 
        "background: linear-gradient(to bottom, rgba(255,255,255,0.1), rgba(255,255,255,0.0)); "
        "border: 1px solid rgba(255, 255, 255, 0.2); color: #fff; font-size: 16px; "
        "cursor: pointer; padding: 4px; margin: 4px; border-radius: 4px; "
        "transition: all 0.1s; box-shadow: 0 2px 4px rgba(0,0,0,0.3);"
    );
    set_text_content(icon);
    
    if (!tooltip.empty()) {
        set_attribute(ctx_->register_string("title"), tooltip);
    }
    
    add_event_listener(ctx_->register_string("mouseover"), [this](const Event& e) {
        set_style("background", "linear-gradient(to bottom, rgba(255,255,255,0.2), rgba(255,255,255,0.1))");
        set_style("border", "1px solid rgba(255, 255, 255, 0.4)");
    });
    add_event_listener(ctx_->register_string("mouseout"), [this](const Event& e) {
        set_style("background", "linear-gradient(to bottom, rgba(255,255,255,0.1), rgba(255,255,255,0.0))");
        set_style("border", "1px solid rgba(255, 255, 255, 0.2)");
        set_style("transform", "scale(1.0)");
        set_style("box-shadow", "0 2px 4px rgba(0,0,0,0.3)");
    });
    add_event_listener(ctx_->strings.mousedown, [this](const Event& e) {
        set_style("transform", "scale(0.95)");
        set_style("box-shadow", "inset 0 2px 4px rgba(0,0,0,0.5)");
        set_style("background", "rgba(0,0,0,0.2)");
    });
    add_event_listener(ctx_->strings.mouseup, [this](const Event& e) {
        set_style("transform", "scale(1.0)");
        set_style("box-shadow", "0 2px 4px rgba(0,0,0,0.3)");
        set_style("background", "linear-gradient(to bottom, rgba(255,255,255,0.2), rgba(255,255,255,0.1))");
    });
}

Toolbar::Toolbar(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, 
        "display: flex; align-items: center; padding: 2px; "
        "border-bottom: 1px solid rgba(255, 255, 255, 0.1); "
        "z-index: 99998;"
    );
}

void Toolbar::add_button(std::shared_ptr<ToolbarButton> button) {
    append_child(button);
}

std::shared_ptr<Toolbar> Toolbar::createFromTemplate(Context* ctx, const ToolbarConfig& config) {
    auto tb = std::make_shared<Toolbar>(ctx);
    ctx->register_element(tb);
    
    std::string direction = config.is_vertical ? "column" : "row";
    
    tb->set_styles({
        {"flex-direction", direction},
        {"background-color", config.bg_color},
        {"position", "absolute"},
        {"left", config.is_vertical ? "0" : "0"},
        {"right", config.is_vertical ? "auto" : "0"},
        {"top", config.is_vertical ? "32px" : "32px"},
        {"bottom", config.is_vertical ? "32px" : "auto"},
        {"width", config.is_vertical ? "40px" : "auto"},
        {"height", config.is_vertical ? "auto" : "40px"}
    });

    for (const auto& btn_conf : config.buttons) {
        auto btn = std::make_shared<ToolbarButton>(ctx, btn_conf.icon, btn_conf.tooltip);
        ctx->register_element(btn);
        if (btn_conf.on_click) {
            btn->add_event_listener(ctx->strings.click, [cb = btn_conf.on_click](const Event& e) {
                cb();
            });
        }
        tb->add_button(btn);
    }

    return tb;
}

} // namespace NetzWirbel
