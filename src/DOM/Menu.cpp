/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "NetzWirbel/DOM/Menu.hpp"
#include "NetzWirbel/Context.hpp"
#include <iostream>

namespace NetzWirbel {

MenuItem::MenuItem(Context* ctx, const std::string& text, const std::string& icon, const std::string& accelerator, bool is_breaker)
    : HTMLDivElement(ctx), is_breaker_(is_breaker) {
    
    if (is_breaker) {
        set_attribute(ctx_->strings.style, "height: 1px; background-color: rgba(255, 255, 255, 0.2); margin: 4px 0;");
    } else {
        set_attribute(ctx_->strings.style, 
            "display: flex; align-items: center; padding: 6px 12px; cursor: pointer; color: #fff; "
            "font-size: 14px; user-select: none; transition: background-color 0.1s;"
        );
        
        auto icon_el = std::make_shared<HTMLSpanElement>(ctx_);
        ctx_->register_element(icon_el);
        icon_el->set_attribute(ctx_->strings.style, "width: 16px; margin-right: 8px; text-align: center;");
        icon_el->set_text_content(icon);
        append_child(icon_el);

        auto text_el = std::make_shared<HTMLSpanElement>(ctx_);
        ctx_->register_element(text_el);
        text_el->set_attribute(ctx_->strings.style, "flex-grow: 1;");
        text_el->set_text_content(text);
        append_child(text_el);

        if (!accelerator.empty()) {
            auto acc_el = std::make_shared<HTMLSpanElement>(ctx_);
            ctx_->register_element(acc_el);
            acc_el->set_attribute(ctx_->strings.style, "color: rgba(255, 255, 255, 0.5); font-size: 12px; margin-left: 16px;");
            acc_el->set_text_content(accelerator);
            append_child(acc_el);
        }

        add_event_listener(ctx_->strings.click, [this](const Event& e) {
            if (on_click_) on_click_();
        });
        
        add_event_listener(ctx_->register_string("mouseover"), [this](const Event& e) {
            set_style("background-color", "rgba(255, 255, 255, 0.1)");
        });
        
        add_event_listener(ctx_->register_string("mouseout"), [this](const Event& e) {
            set_style("background-color", "transparent");
        });
    }
}

void MenuItem::set_submenu(std::shared_ptr<Menu> submenu) {
    submenu_ = submenu;
    if (submenu) {
        auto arrow = std::make_shared<HTMLSpanElement>(ctx_);
        ctx_->register_element(arrow);
        arrow->set_attribute(ctx_->strings.style, "margin-left: 8px; font-size: 10px;");
        arrow->set_text_content("▶");
        append_child(arrow);
    }
}

Menu::Menu(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, 
        "position: absolute; display: none; flex-direction: column; "
        "background-color: rgba(30, 30, 30, 0.95); backdrop-filter: blur(10px); "
        "border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 4px; "
        "box-shadow: 0 4px 12px rgba(0, 0, 0, 0.5); padding: 4px 0; z-index: 99999; "
        "min-width: 150px;"
    );
}

void Menu::add_item(std::shared_ptr<MenuItem> item) {
    items_.push_back(item);
    append_child(item);
    
    // Hide menu when an item is clicked
    if (!item->is_breaker()) {
        item->add_event_listener(ctx_->strings.click, [this](const Event& e) {
            this->hide();
        });
    }

    if (item->get_submenu()) {
        add_submenu(item->get_submenu());
        item->get_submenu()->set_parent_menu(this);
        
        // Handle hovering to show submenu
        // In this basic version, we rely on click.
        item->add_event_listener(ctx_->strings.click, [this, item](const Event& e) {
            hide_all_submenus();
            // Approximate position for submenu. Should query layout, but we'll use a fixed offset.
            item->get_submenu()->show_at(150, 0); // Relative to viewport if parent is body, but actually needs real layout engine
            // In Netzwirbel, absolute is relative to page. We will hack a generic offset.
            // Better: just assume fixed size for demo.
        });
    }
}

void Menu::add_submenu(std::shared_ptr<Menu> submenu) {
    submenus_.push_back(submenu);
}

void Menu::show_at(int x, int y) {
    set_styles({
        {"display", "flex"},
        {"left", std::to_string(x) + "px"},
        {"top", std::to_string(y) + "px"}
    });
}

void Menu::hide() {
    set_style("display", "none");
    hide_all_submenus();
    if (on_hide_) on_hide_();
}

bool Menu::is_visible() const {
    // Actually we'd need to query, but let's assume if it's not "none"
    return true; // Simplified
}

void Menu::hide_all_submenus() {
    for (auto& submenu : submenus_) {
        submenu->hide();
    }
}

MenuBar::MenuBar(Context* ctx) : HTMLDivElement(ctx) {
    set_attribute(ctx_->strings.style, 
        "position: absolute; top: 0; left: 0; right: 0; height: 32px; "
        "background-color: rgba(20, 20, 20, 0.95); backdrop-filter: blur(10px); "
        "display: flex; align-items: center; padding: 0 16px; border-bottom: 1px solid rgba(255, 255, 255, 0.1); "
        "z-index: 99999;"
    );

    backdrop_ = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(backdrop_);
    backdrop_->set_attribute(ctx_->strings.style, 
        "position: fixed; top: 0; left: 0; right: 0; bottom: 0; z-index: 99998; display: none;"
    );
    Command cmd;
    cmd.type = CommandType::APPEND_CHILD;
    cmd.target_id = 0;
    cmd.arg1 = backdrop_->get_id();
    ctx_->send_command(cmd);

    backdrop_->add_event_listener(ctx_->strings.mousedown, [this](const Event& e) {
        if (active_menu_) {
            active_menu_->hide();
        }
    });
}

void MenuBar::add_menu(const std::string& title, std::shared_ptr<Menu> menu) {
    auto title_el = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(title_el);
    title_el->set_attribute(ctx_->strings.style, 
        "padding: 0 12px; height: 100%; display: flex; align-items: center; "
        "color: #ccc; cursor: pointer; font-size: 14px; user-select: none;"
    );
    title_el->set_text_content(title);
    append_child(title_el);

    title_el->add_event_listener(ctx_->register_string("mouseover"), [title_el](const Event& e) {
        title_el->set_style("background-color", "rgba(255, 255, 255, 0.1)");
    });
    
    title_el->add_event_listener(ctx_->register_string("mouseout"), [title_el](const Event& e) {
        title_el->set_style("background-color", "transparent");
    });

    menus_.push_back({title_el, menu});

    menu->set_on_hide([this, menu]() {
        if (active_menu_ == menu) {
            active_menu_ = nullptr;
            is_active_ = false;
            backdrop_->set_style("display", "none");
        }
    });

    title_el->add_event_listener(ctx_->strings.mousedown, [this, title_el, menu](const Event& e) {
        if (active_menu_ == menu) {
            menu->hide();
        } else {
            if (active_menu_) active_menu_->hide();
            active_menu_ = menu;
            is_active_ = true;
            backdrop_->set_style("display", "block");
            
            int idx = 0;
            for (size_t i = 0; i < menus_.size(); ++i) {
                if (menus_[i].first == title_el) idx = i;
            }
            menu->show_at(16 + idx * 60, 32); 
        }
    });

    // Also need to append the menu to the document body so it renders on top.
    Command cmd;
    cmd.type = CommandType::APPEND_CHILD;
    cmd.target_id = 0;
    cmd.arg1 = menu->get_id();
    ctx_->send_command(cmd);
}

ContextMenu::ContextMenu(Context* ctx) : Menu(ctx) {
}

void ContextMenu::attach_to(std::shared_ptr<Element> target) {
    // In a real framework we'd bind right-click (contextmenu event).
    // We will bind mousedown with right button or just left button for demo purposes.
    target->add_event_listener(ctx_->strings.mousedown, [this](const Event& e) {
        if (auto* me = dynamic_cast<const MouseEvent*>(&e)) {
            // Simplified: always show on click. Normally we check me->button()
            show_at(me->get_client_x(), me->get_client_y());
        }
    });
}

} // namespace NetzWirbel
