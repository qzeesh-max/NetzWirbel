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
#include <functional>
#include <string>
#include <memory>
#include <vector>

namespace NetzWirbel {

class Menu;

class MenuItem : public HTMLDivElement {
public:
    MenuItem(Context* ctx, const std::string& text, const std::string& icon = "", const std::string& accelerator = "", bool is_breaker = false);
    
    void set_on_click(std::function<void()> cb) { on_click_ = cb; }
    void set_submenu(std::shared_ptr<Menu> submenu);

    bool is_breaker() const { return is_breaker_; }
    std::shared_ptr<Menu> get_submenu() const { return submenu_; }

private:
    std::function<void()> on_click_;
    std::shared_ptr<Menu> submenu_;
    bool is_breaker_;
};

class Menu : public HTMLDivElement {
public:
    Menu(Context* ctx);
    
    void add_item(std::shared_ptr<MenuItem> item);
    void show_at(int x, int y);
    void hide();
    bool is_visible() const;

    // To handle nesting
    void set_parent_menu(Menu* parent) { parent_menu_ = parent; }
    void hide_all_submenus();
    void add_submenu(std::shared_ptr<Menu> submenu);
    void set_on_hide(std::function<void()> cb) { on_hide_ = cb; }

private:
    Menu* parent_menu_ = nullptr;
    std::vector<std::shared_ptr<MenuItem>> items_;
    std::vector<std::shared_ptr<Menu>> submenus_;
    std::function<void()> on_hide_;
};

class MenuBar : public HTMLDivElement {
public:
    MenuBar(Context* ctx);
    
    void add_menu(const std::string& title, std::shared_ptr<Menu> menu);

private:
    std::vector<std::pair<std::shared_ptr<HTMLDivElement>, std::shared_ptr<Menu>>> menus_;
    std::shared_ptr<Menu> active_menu_ = nullptr;
    bool is_active_ = false;
    std::shared_ptr<HTMLDivElement> backdrop_;
};

class ContextMenu : public Menu {
public:
    ContextMenu(Context* ctx);
    void attach_to(std::shared_ptr<Element> target);
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
