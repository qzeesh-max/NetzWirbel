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

#include "NetzWirbel/DOM/Window.hpp"
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Button.hpp"
#include "NetzWirbel/DOM/Event.hpp"
#include <sstream>
#include <algorithm>


namespace NetzWirbel {
    std::function<void(double)> g_grid_mousemove;
    std::function<void()> g_grid_mouseup;


int Window::global_z_index_ = 100;
std::vector<Window*> WindowManager::active_windows_;
Window* WindowManager::active_drag_win_ = nullptr;
Window* WindowManager::active_resize_win_ = nullptr;
bool WindowManager::is_attached_ = false;

void WindowManager::attach(std::shared_ptr<Element> root_container, Context* ctx) {
    if (is_attached_) return;
    is_attached_ = true;

    root_container->add_event_listener(ctx->strings.mousemove, [ctx](const Event& ev) {
        if (auto* me = dynamic_cast<const MouseEvent*>(&ev)) {
            if (g_grid_mousemove) {
                g_grid_mousemove(me->get_client_x());
            }
        }
        if (active_drag_win_ == nullptr && active_resize_win_ == nullptr) return;
        
        const MouseEvent* mev = dynamic_cast<const MouseEvent*>(&ev);
        if (!mev) return;

        if (active_drag_win_ && active_drag_win_->is_dragging_) {
            auto w = active_drag_win_;
            if (w->maximized_) return;
            int dx = mev->get_client_x() - w->drag_start_x_;
            int dy = mev->get_client_y() - w->drag_start_y_;
            w->x_ = w->initial_win_x_ + dx;
            w->y_ = w->initial_win_y_ + dy;
            w->apply_styles();
        }

        if (active_resize_win_ && active_resize_win_->is_resizing_) {
            auto w = active_resize_win_;
            if (w->maximized_) return;
            int dx = mev->get_client_x() - w->drag_start_x_;
            int dy = mev->get_client_y() - w->drag_start_y_;
            
            // Calculate new bounds based on resize_mode_
            int nx = w->initial_win_x_;
            int ny = w->initial_win_y_;
            int nw = w->initial_win_w_;
            int nh = w->initial_win_h_;

            if (w->resize_mode_ == 1 || w->resize_mode_ == 5 || w->resize_mode_ == 6) { // N
                nh -= dy;
                ny += dy;
            }
            if (w->resize_mode_ == 2 || w->resize_mode_ == 7 || w->resize_mode_ == 8) { // S
                nh += dy;
            }
            if (w->resize_mode_ == 3 || w->resize_mode_ == 5 || w->resize_mode_ == 7) { // E
                nw += dx;
            }
            if (w->resize_mode_ == 4 || w->resize_mode_ == 6 || w->resize_mode_ == 8) { // W
                nw -= dx;
                nx += dx;
            }

            if (nw >= 250) { w->x_ = nx; w->width_ = nw; }
            if (nh >= 150) { w->y_ = ny; w->height_ = nh; }
            
            w->apply_styles();
        }
    });

    root_container->add_event_listener(ctx->strings.mouseup, [](const Event& ev) {
        if (g_grid_mouseup) g_grid_mouseup();
        bool drag_saved = false;
        bool resize_saved = false;
        
        if (active_drag_win_) {
            active_drag_win_->is_dragging_ = false;
            drag_saved = true;
        }
        if (active_resize_win_) {
            active_resize_win_->is_resizing_ = false;
            resize_saved = true;
        }

        auto dragged_win = active_drag_win_;
        auto resized_win = active_resize_win_;
        active_drag_win_ = nullptr;
        active_resize_win_ = nullptr;

        if (drag_saved && dragged_win && dragged_win->on_move_) {
            dragged_win->on_move_(dragged_win->x_, dragged_win->y_);
        }
        if (resize_saved && resized_win && resized_win->on_resize_) {
            resized_win->on_resize_(resized_win->width_, resized_win->height_);
        }
    });
}

void WindowManager::register_window(Window* w) {
    active_windows_.push_back(w);
}

void WindowManager::unregister_window(Window* w) {
    active_windows_.erase(std::remove(active_windows_.begin(), active_windows_.end(), w), active_windows_.end());
    if (active_drag_win_ == w) active_drag_win_ = nullptr;
    if (active_resize_win_ == w) active_resize_win_ = nullptr;
}

void Window::attach_manager(std::shared_ptr<Element> root_container, Context* ctx) {
    WindowManager::attach(root_container, ctx);
}

Window::Window(Context* ctx, const std::string& id, const std::string& title, int x, int y, int width, int height)
    : HTMLDivElement(ctx), id_(id), x_(x), y_(y), width_(width), height_(height), z_index_(global_z_index_++) {
    
    saved_x_ = x_; saved_y_ = y_; saved_width_ = width_; saved_height_ = height_;

    WindowManager::register_window(this);
    apply_styles();

    add_event_listener(ctx_->register_string("mousedown"), [this](const Event& e) {
        bring_to_front();
    });

    // Header (Title Bar)
    header_ = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(header_);
    header_->set_attribute(ctx_->strings.style, 
        "display: flex; align-items: center; justify-content: space-between; "
        "padding: 0 0 0 12px; background: rgba(0,0,0,0.05); border-bottom: 1px solid rgba(0,0,0,0.1); "
        "cursor: move; height: 32px;"
    );
    append_child(header_);

    // Title Text
    title_el_ = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
    ctx_->register_element(title_el_);
    title_el_->set_text_content(ctx_->register_string(title));
    title_el_->set_attribute(ctx_->strings.style, 
        "font-weight: bold; color: #333; font-size: 14px; "
        "white-space: nowrap; overflow: hidden; text-overflow: ellipsis; flex-grow: 1; margin-right: 8px;"
    );
    header_->append_child(title_el_);

    // Button Container
    button_container_ = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(button_container_);
    update_button_container_visibility();
    header_->append_child(button_container_);

    btn_min_ = std::make_shared<Button>(ctx_);
    ctx_->register_element(btn_min_);
    std::static_pointer_cast<Button>(btn_min_)->set_icon("-");
    button_container_->append_child(btn_min_);

    btn_max_ = std::make_shared<Button>(ctx_);
    ctx_->register_element(btn_max_);
    std::static_pointer_cast<Button>(btn_max_)->set_icon("□");
    button_container_->append_child(btn_max_);

    btn_close_ = std::make_shared<Button>(ctx_);
    ctx_->register_element(btn_close_);
    std::static_pointer_cast<Button>(btn_close_)->set_icon("x");
    button_container_->append_child(btn_close_);

    btn_min_->add_event_listener(ctx_->strings.click, [this](const Event& e) {
        set_minimized(true);
    });
    
    btn_max_->add_event_listener(ctx_->strings.click, [this](const Event& e) {
        set_maximized(!maximized_);
    });
    
    btn_close_->add_event_listener(ctx_->strings.click, [this](const Event& e) {
        set_visible(false);
        if (on_close_) on_close_();
    });

    // Content Container
    content_container_ = std::make_shared<HTMLDivElement>(ctx_);
    ctx_->register_element(content_container_);
    content_container_->set_attribute(ctx_->strings.style, 
        "padding: 12px; flex-grow: 1; min-height: 0; box-sizing: border-box; overflow: auto; display: flex; flex-direction: column;"
    );
    append_child(content_container_);

    header_->add_event_listener(ctx_->register_string("mousedown"), [this](const Event& e) {
        if (auto* me = dynamic_cast<const MouseEvent*>(&e)) {
            if (maximized_) return;
            is_dragging_ = true;
            drag_start_x_ = me->get_client_x();
            drag_start_y_ = me->get_client_y();
            initial_win_x_ = x_;
            initial_win_y_ = y_;
            bring_to_front();
            WindowManager::active_drag_win_ = this;
        }
    });

    // Add Resize Handles (8 directions)
    const std::vector<std::pair<int, std::string>> handles = {
        {1, "top: -4px; left: 8px; right: 8px; height: 8px; cursor: n-resize;"},
        {2, "bottom: -4px; left: 8px; right: 8px; height: 8px; cursor: s-resize;"},
        {3, "top: 8px; bottom: 8px; right: -4px; width: 8px; cursor: e-resize;"},
        {4, "top: 8px; bottom: 8px; left: -4px; width: 8px; cursor: w-resize;"},
        {5, "top: -4px; right: -4px; width: 12px; height: 12px; cursor: ne-resize; z-index: 2;"},
        {6, "top: -4px; left: -4px; width: 12px; height: 12px; cursor: nw-resize; z-index: 2;"},
        {7, "bottom: -4px; right: -4px; width: 12px; height: 12px; cursor: se-resize; z-index: 2;"},
        {8, "bottom: -4px; left: -4px; width: 12px; height: 12px; cursor: sw-resize; z-index: 2;"}
    };

    for (const auto& h : handles) {
        auto handle_el = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(handle_el);
        handle_el->set_attribute(ctx_->strings.style, "position: absolute; " + h.second);
        int mode = h.first;
        handle_el->add_event_listener(ctx_->register_string("mousedown"), [this, mode](const Event& e) {
            if (auto* me = dynamic_cast<const MouseEvent*>(&e)) {
                is_resizing_ = true;
                resize_mode_ = mode;
                drag_start_x_ = me->get_client_x();
                drag_start_y_ = me->get_client_y();
                initial_win_x_ = x_;
                initial_win_y_ = y_;
                initial_win_w_ = width_;
                initial_win_h_ = height_;
                bring_to_front();
                WindowManager::active_resize_win_ = this;
            }
        });
        append_child(handle_el);
    }
}

Window::~Window() {
    WindowManager::unregister_window(this);
}

void Window::set_title(const std::string& title) {
    title_el_->set_text_content(ctx_->register_string(title));
}

void Window::apply_styles() {
    std::stringstream ss;
    if (maximized_) {
        ss << "position: absolute; left: 0px; top: 40px; width: 100%; height: calc(100% - 40px); "
           << "background: rgba(255, 255, 255, 0.95); backdrop-filter: blur(10px); "
           << "-webkit-backdrop-filter: blur(10px); border-radius: 0; "
           << "border: none; z-index: " << (is_modal_ ? z_index_ + 10000 : z_index_) << "; display: " << (visible_ && !minimized_ ? "flex" : "none") << "; "
           << "flex-direction: column;";
    } else {
        ss << "position: absolute; "
           << "left: " << x_ << "px; top: " << y_ << "px; "
           << "width: " << width_ << "px; height: " << height_ << "px; "
           << "background: rgba(255, 255, 255, 0.7); backdrop-filter: blur(10px); "
           << "-webkit-backdrop-filter: blur(10px); border-radius: 12px; "
           << "border: 1px solid rgba(255,255,255,0.4); "
           << "box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.15); "
           << "display: " << (visible_ && !minimized_ ? "flex" : "none") << "; "
           << "flex-direction: column; "
           << "z-index: " << (is_modal_ ? z_index_ + 10000 : z_index_) << "; ";
    }
    set_attribute(ctx_->strings.style, ss.str());
}

void Window::set_bounds(int x, int y, int width, int height) {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
    apply_styles();
    
    this->add_event_listener("mousedown", [this](const Event&) {
        bring_to_front();
    });
}
void Window::set_visible(bool visible) {
    visible_ = visible;
    apply_styles();
    
    this->add_event_listener("mousedown", [this](const Event&) {
        bring_to_front();
    });
}

void Window::set_minimized(bool minimized) {
    minimized_ = minimized;
    apply_styles();
    if (!minimized) {
        bring_to_front();
    }
    if (on_minimize_) on_minimize_(minimized);
}

void Window::set_maximized(bool maximized) {
    maximized_ = maximized;
    apply_styles();
    if (maximized_) {
        std::static_pointer_cast<Button>(btn_max_)->set_icon("❐");
    } else {
        std::static_pointer_cast<Button>(btn_max_)->set_icon("□");
    }
}

void Window::set_minimize_button_enabled(bool enabled) { std::static_pointer_cast<Button>(btn_min_)->set_disabled(!enabled); }
void Window::set_maximize_button_enabled(bool enabled) { std::static_pointer_cast<Button>(btn_max_)->set_disabled(!enabled); }
void Window::set_close_button_enabled(bool enabled) { std::static_pointer_cast<Button>(btn_close_)->set_disabled(!enabled); }

void Window::set_minimize_button_hidden(bool hidden) {
    min_hidden_ = hidden;
    btn_min_->set_attribute(ctx_->strings.style, hidden ? "display: none;" : "");
    update_button_container_visibility();
}

void Window::set_maximize_button_hidden(bool hidden) {
    max_hidden_ = hidden;
    btn_max_->set_attribute(ctx_->strings.style, hidden ? "display: none;" : "");
    update_button_container_visibility();
}

void Window::set_close_button_hidden(bool hidden) {
    close_hidden_ = hidden;
    btn_close_->set_attribute(ctx_->strings.style, hidden ? "display: none;" : "");
    update_button_container_visibility();
}

void Window::update_button_container_visibility() {
    if (min_hidden_ && max_hidden_ && close_hidden_) {
        button_container_->set_attribute(ctx_->strings.style, "display: none;");
    } else {
        button_container_->set_attribute(ctx_->strings.style, 
            "display: flex; align-items: center; gap: 4px; padding: 0 8px 0 16px; height: 100%; "
            "background: linear-gradient(135deg, #f0f0f0 0%, #c0c0c0 50%, #f0f0f0 100%); "
            "border-top-left-radius: 20px; border-bottom-left-radius: 20px; "
            "border-left: 1px solid rgba(255,255,255,0.7);"
        );
    }
}

void Window::bring_to_front() {
    z_index_ = global_z_index_++;
    apply_styles();
    
    this->add_event_listener("mousedown", [this](const Event&) {
        bring_to_front();
    });
}

} // namespace NetzWirbel

void NetzWirbel::Window::set_modal(bool modal) {
    is_modal_ = modal;
    apply_styles();
}
