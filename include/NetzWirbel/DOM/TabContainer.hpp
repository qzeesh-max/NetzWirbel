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

class Tab : public HTMLDivElement {
public:
    Tab(Context* ctx, const std::string& title);
    
    void set_active(bool active);
    void set_on_click(std::function<void()> cb) { on_click_ = cb; }

    // For drag and drop reordering
    void set_drag_handlers(std::function<void(const Event&)> dragstart,
                           std::function<void(const Event&)> dragover,
                           std::function<void(const Event&)> drop);

private:
    std::function<void()> on_click_;
    bool is_active_ = false;
};

class TabPanel : public HTMLDivElement {
public:
    TabPanel(Context* ctx);
    void set_active(bool active);
};

class TabContainer : public HTMLDivElement {
public:
    TabContainer(Context* ctx);
    
    void add_tab(const std::string& title, std::shared_ptr<Element> content);
    void select_tab(size_t index);
    
    void set_on_state_change(std::function<void(size_t)> cb) { on_state_change_ = cb; }
    size_t get_active_index() const { return active_index_; }

    void reorder_tabs(size_t from_index, size_t to_index);

    // To be called globally when an accelerator is pressed (e.g. Alt+1)
    bool handle_accelerator(const std::string& key, uint8_t modifiers);

private:
    std::shared_ptr<HTMLDivElement> header_;
    std::shared_ptr<HTMLDivElement> content_area_;
    
    std::vector<std::pair<std::shared_ptr<Tab>, std::shared_ptr<TabPanel>>> tabs_;
    size_t active_index_ = 0;
    
    std::function<void(size_t)> on_state_change_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
