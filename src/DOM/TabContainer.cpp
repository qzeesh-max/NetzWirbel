/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "NetzWirbel/DOM/TabContainer.hpp"
#include "NetzWirbel/Context.hpp"

namespace NetzWirbel {

Tab::Tab(Context* ctx, const std::string& title) : HTMLDivElement(ctx) {
    set_text_content(title);
    set_active(false);
    
    // We would use actual HTML5 drag and drop if supported, but here we can just use simple clicks or custom dragging.
    // Assuming draggable attribute is supported in DOM bridge (requires adding it, but we can fake it or just rely on mouse events).
    set_attribute(ctx_->register_string("draggable"), "true");
    
    add_event_listener(ctx_->strings.click, [this](const Event& e) {
        if (on_click_) on_click_();
    });
}

void Tab::set_active(bool active) {
    is_active_ = active;
    if (active) {
        set_attribute(ctx_->strings.style, 
            "padding: 8px 16px; background-color: #333; color: #fff; "
            "border-top: 2px solid #00e5ff; cursor: pointer; user-select: none; "
            "border-right: 1px solid #222; font-size: 14px;"
        );
    } else {
        set_attribute(ctx_->strings.style, 
            "padding: 8px 16px; background-color: #222; color: #aaa; "
            "border-top: 2px solid transparent; cursor: pointer; user-select: none; "
            "border-right: 1px solid #111; font-size: 14px;"
        );
    }
}

void Tab::set_drag_handlers(std::function<void(const Event&)> dragstart,
                            std::function<void(const Event&)> dragover,
                            std::function<void(const Event&)> drop) {
    add_event_listener(ctx_->register_string("dragstart"), dragstart);
    add_event_listener(ctx_->register_string("dragover"), dragover, true); // prevent default to allow drop
    add_event_listener(ctx_->register_string("drop"), drop);
}

TabPanel::TabPanel(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, "flex-grow: 1; display: none; overflow: auto; background-color: #333;");
}

void TabPanel::set_active(bool active) {
    set_style("display", active ? "block" : "none");
}

TabContainer::TabContainer(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, "display: flex; flex-direction: column; width: 100%; height: 100%;");
    
    header_ = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(header_);
    header_->set_attribute(ctx_->strings.style, "display: flex; background-color: #1a1a1a;");
    append_child(header_);
    
    content_area_ = std::make_shared<HTMLDivElement>(ctx);
    ctx->register_element(content_area_);
    content_area_->set_attribute(ctx_->strings.style, "flex-grow: 1; display: flex; flex-direction: column;");
    append_child(content_area_);
}

void TabContainer::add_tab(const std::string& title, std::shared_ptr<Element> content) {
    size_t index = tabs_.size();
    
    auto tab = std::make_shared<Tab>(ctx_, title);
    ctx_->register_element(tab);
    
    auto panel = std::make_shared<TabPanel>(ctx_);
    ctx_->register_element(panel);
    panel->append_child(content);
    
    tab->set_on_click([this, index]() {
        select_tab(index);
    });

    // Drag and drop setup
    uint32_t tab_id = tab->get_id();
    tab->set_drag_handlers(
        [tab_id, this](const Event& e) {
            // In a full implementation, we'd use dataTransfer. For now we track globally or rely on ID.
            // A simple hack is to store it in a static, but that breaks multiple containers.
            // For now, we will just use the event target_id if we have it, but EventMsg doesn't expose it directly yet.
            // Let's assume reordering works by dropping one tab onto another.
        },
        [](const Event& e) {}, // dragover (prevented default in Tab)
        [this, index](const Event& e) {
            // Real implementation would parse dataTransfer.
            // This is a placeholder for the logic.
        }
    );

    tabs_.push_back({tab, panel});
    header_->append_child(tab);
    content_area_->append_child(panel);
    
    if (index == 0) {
        select_tab(0);
    } else {
        tab->set_active(false);
        panel->set_active(false);
    }
}

void TabContainer::select_tab(size_t index) {
    if (index >= tabs_.size()) return;
    
    for (size_t i = 0; i < tabs_.size(); ++i) {
        bool is_active = (i == index);
        tabs_[i].first->set_active(is_active);
        tabs_[i].second->set_active(is_active);
    }
    active_index_ = index;
    
    if (on_state_change_) {
        on_state_change_(index);
    }
}

void TabContainer::reorder_tabs(size_t from_index, size_t to_index) {
    if (from_index >= tabs_.size() || to_index >= tabs_.size() || from_index == to_index) return;
    
    auto item = tabs_[from_index];
    tabs_.erase(tabs_.begin() + from_index);
    tabs_.insert(tabs_.begin() + to_index, item);
    
    // Update DOM order
    // In Netzwirbel, we don't have insertBefore easily yet. We would append all again.
    // Not fully supported by the current bridge without remove_child and append_child loops.
}

bool TabContainer::handle_accelerator(const std::string& key, uint8_t modifiers) {
    // E.g. Alt+1 for tab 0, Alt+2 for tab 1...
    if ((modifiers & Event::MODIFIER_ALT) != 0) {
        if (key.length() == 1 && key[0] >= '1' && key[0] <= '9') {
            size_t idx = key[0] - '1';
            if (idx < tabs_.size()) {
                select_tab(idx);
                return true;
            }
        }
    }
    return false;
}

} // namespace NetzWirbel
