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
#include <vector>
#include <functional>
#include <memory>

namespace NetzWirbel {

class Button;

class Window : public HTMLDivElement {
public:
    Window(Context* ctx, const std::string& id, const std::string& title, int x, int y, int width, int height);
    virtual ~Window();

    std::shared_ptr<Element> get_content_container() const { return content_container_; }
    std::shared_ptr<Element> get_header() const { return header_; }
    
    void set_title(const std::string& title);
    std::string get_window_id() const { return id_; }
    
    void set_visible(bool visible);
    bool is_visible() const { return visible_; }
    
    void set_minimized(bool minimized);
    void set_hide_on_minimize(bool hide) { hide_on_minimize_ = hide; }
    void set_on_minimize(std::function<void(bool)> cb) { on_minimize_ = cb; }
    void set_on_maximize(std::function<void(bool)> cb) { on_maximize_ = cb; }
    bool is_minimized() const { return minimized_; }
    
    void set_maximized(bool maximized);
    void set_modal(bool modal);
    bool is_maximized() const { return maximized_; }

    void set_minimize_button_enabled(bool enabled);
    void set_maximize_button_enabled(bool enabled);
    void set_close_button_enabled(bool enabled);

    void set_minimize_button_hidden(bool hidden);
    void set_maximize_button_hidden(bool hidden);
    void set_close_button_hidden(bool hidden);

    // Callbacks for layout saving
    void set_on_move(std::function<void(int, int)> cb) { on_move_ = cb; }
    void set_on_resize(std::function<void(int, int)> cb) { on_resize_ = cb; }
    void set_on_close(std::function<void()> cb) { on_close_ = cb; }
    void set_destroy_on_close(bool destroy) { destroy_on_close_ = destroy; }

    int get_x() const { return x_; }
    int get_y() const { return y_; }
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    int get_z_index() const { return z_index_; }
    void set_bounds(int x, int y, int width, int height);
    
    void bring_to_front();
    
    static void attach_manager(std::shared_ptr<Element> root_container, Context* ctx);

private:
    void apply_styles();
    void update_button_container_visibility();
    
    std::string id_;
    int x_, y_, width_, height_, z_index_;
    int saved_x_, saved_y_, saved_width_, saved_height_;
    bool visible_ = true;
    bool minimized_ = false;
    bool maximized_ = false;
    bool is_modal_ = false;

    bool min_hidden_ = false;
    bool max_hidden_ = false;
    bool close_hidden_ = false;

    std::shared_ptr<Element> header_;
    std::shared_ptr<Element> title_el_;
    std::shared_ptr<Element> button_container_;
    std::shared_ptr<Button> btn_min_;
    std::shared_ptr<Button> btn_max_;
    std::shared_ptr<Button> btn_close_;
    std::shared_ptr<Element> content_container_;

    // Resize handles
    std::shared_ptr<Element> resize_n, resize_s, resize_e, resize_w;
    std::shared_ptr<Element> resize_ne, resize_nw, resize_se, resize_sw;

    std::function<void(int, int)> on_move_;
    std::function<void(int, int)> on_resize_;
    std::function<void()> on_close_;
    std::function<void(bool)> on_minimize_;
    std::function<void(bool)> on_maximize_;

    static int global_z_index_;
    
    // For routing dragging and resizing
    friend class WindowManager;
    
    bool is_dragging_ = false;
    bool is_resizing_ = false;
    int resize_mode_ = 0; // 1=N, 2=S, 3=E, 4=W, 5=NE, 6=NW, 7=SE, 8=SW
    int drag_start_x_ = 0, drag_start_y_ = 0;
    int initial_win_x_ = 0, initial_win_y_ = 0;
    int initial_win_w_ = 0, initial_win_h_ = 0;
    bool hide_on_minimize_ = false;
    bool destroy_on_close_ = true;
};

class WindowManager {
public:
    static void attach(std::shared_ptr<Element> root_container, Context* ctx);
    static void register_window(Window* w);
    static void unregister_window(Window* w);
    static std::vector<Window*> active_windows_;
    static Window* active_drag_win_;
    static Window* active_resize_win_;
    static bool is_attached_;

    static void set_margin_top(int margin) { margin_top_ = margin; }
    static void set_margin_bottom(int margin) { margin_bottom_ = margin; }
    static void set_margin_left(int margin) { margin_left_ = margin; }
    static void set_margin_right(int margin) { margin_right_ = margin; }

    static int margin_top_;
    static int margin_bottom_;
    static int margin_left_;
    static int margin_right_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
