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
#include "NetzWirbel/DOM/Event.hpp"
#include <string>
#include <vector>
#include <algorithm>

#include <functional>
namespace NetzWirbel {
    extern std::function<void(double)> g_grid_mousemove;
    extern std::function<void()> g_grid_mouseup;
}

#include <memory>
#include <sstream>

namespace NetzWirbel {

enum SortDirection { SORT_NONE, SORT_ASC, SORT_DESC };

struct ColumnDef {
    std::string name;
    int min_width;
    int current_width;
    std::shared_ptr<Element> header_el;
    std::shared_ptr<Element> sort_icon_el;
};

template <typename T>
class GridRow : public HTMLDivElement {
public:
    GridRow(Context* ctx) : HTMLDivElement(ctx) {
        set_attribute(ctx_->strings.style, "display: flex; border-bottom: 1px solid rgba(0,0,0,0.05); width: 100%;");
    }
    
    void update_data(const T& data) {
        data_ = data;
    }
    int seq_ = 0;
    
    T get_data() const { return data_; }

    void add_cell(const std::string& text, int width_px, const std::string& col_name = "") {
        auto cell = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(cell);
        cell->set_text_content(ctx_->register_string(text));
        
        std::stringstream ss;
        ss << "padding: 6px; box-sizing: border-box; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; user-select: none; ";
        if (width_px > 0) ss << "width: " << width_px << "px; min-width: " << width_px << "px; max-width: " << width_px << "px;";
        else ss << "flex: 1;";
        
        cell->set_attribute(ctx_->strings.style, ss.str());
        
        if (!col_name.empty()) {
            cell->add_event_listener("dblclick", [this, col_name, cb = on_double_click_](const Event& e) {
                if (cb) cb(this, col_name);
            });
        }
        
        cells_.push_back(cell);
        append_child(cell);
    }
    
    void set_on_double_click(std::function<void(GridRow<T>*, const std::string&)> cb) {
        on_double_click_ = cb;
    }

    std::shared_ptr<Element> get_cell(size_t index) {
        if (index < cells_.size()) return cells_[index];
        return nullptr;
    }

private:
    T data_;
    std::vector<std::shared_ptr<Element>> cells_;
    std::function<void(GridRow<T>*, const std::string&)> on_double_click_;
};

template <typename T>
class Grid : public HTMLDivElement {
public:
    Grid(Context* ctx) : HTMLDivElement(ctx) {
        set_attribute(ctx_->strings.style, "display: block; width: 100%; flex-grow: 1; min-height: 0; overflow: auto; font-size: 13px;");
        
        header_container_ = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(header_container_);
        header_container_->set_attribute(ctx_->strings.style, "display: flex; font-weight: bold; background: #f8f9fa; border-bottom: 2px solid #ccc; color: #1a237e; min-width: 100%; width: max-content; position: sticky; top: 0; z-index: 10;");
        append_child(header_container_);

        body_container_ = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(body_container_);
        body_container_->set_attribute(ctx_->strings.style, "display: flex; flex-direction: column; flex-grow: 1; min-width: 100%; width: max-content;");
        append_child(body_container_);
        
        // Ensure mouseup/mousemove on window level is tracked for resizing if needed
        // but for simplicity we can track on the grid itself.
        add_event_listener("mousemove", [this](const Event& e) { handle_mousemove(e); });
        add_event_listener("mouseup", [this](const Event& e) { handle_mouseup(e); });
        // mouseleave removed to allow dragging outside
    }
    
    void add_column(const std::string& name, int default_width) {
        ColumnDef cdef;
        cdef.name = name;
        cdef.min_width = 30;
        cdef.current_width = default_width;
        
        auto th = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(th);
        std::stringstream ss;
        ss << "padding: 6px; box-sizing: border-box; position: relative; user-select: none; display: flex; flex-direction: row; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; ";
        if (default_width > 0) ss << "width: " << default_width << "px; min-width: " << default_width << "px; max-width: " << default_width << "px;";
        else ss << "flex: 1;";
        th->set_attribute(ctx_->strings.style, ss.str());
        
        auto label_cont = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(label_cont);
        label_cont->set_attribute(ctx_->strings.style, "display: flex; flex-direction: row; align-items: center; cursor: pointer; flex-grow: 1; overflow: hidden;");
        
        auto span = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
        ctx_->register_element(span);
        span->set_text_content(ctx_->register_string(name));
        span->set_attribute(ctx_->strings.style, "overflow: hidden; text-overflow: ellipsis; white-space: nowrap;");
        label_cont->append_child(span);
        
        auto icon = std::make_shared<Element>(ctx_, ctx_->register_string("span"));
        ctx_->register_element(icon);
        icon->set_attribute(ctx_->strings.style, "margin-left: 4px; font-size: 10px; color: #777; text-shadow: 0px 1px 1px rgba(255,255,255,0.8);");
        label_cont->append_child(icon);
        cdef.sort_icon_el = icon;
        
        int col_idx = cols_.size();
        label_cont->add_event_listener(ctx_->strings.click, [this, col_idx](const Event&) {
            if (resizing_col_ != -1) return;
            if (sort_col_idx_ == col_idx) {
                if (sort_dir_ == SORT_NONE) sort_dir_ = SORT_ASC;
                else if (sort_dir_ == SORT_ASC) sort_dir_ = SORT_DESC;
                else { sort_dir_ = SORT_NONE; sort_col_idx_ = -1; }
            } else {
                sort_col_idx_ = col_idx;
                sort_dir_ = SORT_ASC;
            }
            apply_sort();
        });
        
        th->append_child(label_cont);
        
        // Resizer border
        auto resizer = std::make_shared<HTMLDivElement>(ctx_);
        ctx_->register_element(resizer);
        resizer->set_attribute(ctx_->strings.style, "position: absolute; right: 0; top: 0; bottom: 0; width: 5px; cursor: col-resize; background: rgba(0,0,0,0.1); border-right: 1px solid #ccc;");
        
        resizer->add_event_listener("mousedown", [this, col_idx](const Event& e) {
            auto* me = dynamic_cast<const MouseEvent*>(&e);
            if (!me) return;
            resizing_col_ = col_idx;
            start_x_ = me->get_client_x();
            start_w_ = cols_[col_idx].current_width;
            
            NetzWirbel::g_grid_mousemove = [this](double client_x) {
                if (resizing_col_ >= 0 && resizing_col_ < cols_.size()) {
                    int dx = (int)client_x - (int)start_x_;
                    int new_w = start_w_ + dx;
                    if (new_w < cols_[resizing_col_].min_width) new_w = cols_[resizing_col_].min_width;
                    
                    cols_[resizing_col_].current_width = new_w;
                    
                    std::stringstream ss;
                    ss << "padding: 6px; box-sizing: border-box; position: relative; user-select: none; ";
                    ss << "width: " << new_w << "px; min-width: " << new_w << "px; max-width: " << new_w << "px;";
                    cols_[resizing_col_].header_el->set_attribute(ctx_->strings.style, ss.str());
                    
                    for (auto& row : rows_) {
                        auto cell = row->get_cell(resizing_col_);
                        if (cell) {
                            std::stringstream css;
                            css << "padding: 6px; box-sizing: border-box; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; user-select: none; ";
                            css << "width: " << new_w << "px; min-width: " << new_w << "px; max-width: " << new_w << "px;";
                            cell->set_attribute(ctx_->strings.style, css.str());
                        }
                    }
                }
            };
            
            NetzWirbel::g_grid_mouseup = [this]() {
                resizing_col_ = -1;
                NetzWirbel::g_grid_mousemove = nullptr;
                NetzWirbel::g_grid_mouseup = nullptr;
            };
        });
        
        th->append_child(resizer);
        cdef.header_el = th;
        
        cols_.push_back(cdef);
        header_container_->append_child(th);
    }
    
    void set_on_render_row(std::function<void(std::shared_ptr<GridRow<T>>, const T&)> cb) {
        on_render_row_ = cb;
    }

    void clear_rows() {
        rows_.clear();
        body_container_->set_text_content(ctx_->register_string(""));
    }

    std::shared_ptr<GridRow<T>> add_row(const T& data) {
        auto row = std::make_shared<GridRow<T>>(ctx_);
        ctx_->register_element(row);
        row->update_data(data);
        row->seq_ = ++seq_counter_;
        row->set_on_double_click([this](GridRow<T>* r, const std::string& col) {
            if (on_cell_double_click_) {
                // Find shared_ptr
                for (auto& sptr : rows_) {
                    if (sptr.get() == r) {
                        on_cell_double_click_(sptr, col);
                        break;
                    }
                }
            }
        });
        
        if (on_render_row_) {
            on_render_row_(row, data);
        }
        
        rows_.push_back(row);
        body_container_->append_child(row);
        return row;
    }

    void set_on_cell_double_click(std::function<void(std::shared_ptr<GridRow<T>>, const std::string&)> cb) {
        on_cell_double_click_ = cb;
    }
    
    int get_col_width(size_t idx) const {
        if (idx < cols_.size()) return cols_[idx].current_width;
        return 0;
    }

private:
    void handle_mousemove(const Event& e) {
        if (resizing_col_ >= 0 && resizing_col_ < cols_.size()) {
            auto* me = dynamic_cast<const MouseEvent*>(&e);
            if (!me) return;
            int dx = (int)me->get_client_x() - (int)start_x_;
            int new_w = start_w_ + dx;
            if (new_w < cols_[resizing_col_].min_width) new_w = cols_[resizing_col_].min_width;
            
            cols_[resizing_col_].current_width = new_w;
            
            // Update header
            std::stringstream ss;
            ss << "padding: 6px; box-sizing: border-box; position: relative; user-select: none; display: flex; flex-direction: row; overflow: hidden; white-space: nowrap; text-overflow: ellipsis; ";
            ss << "width: " << new_w << "px; min-width: " << new_w << "px; max-width: " << new_w << "px;";
            cols_[resizing_col_].header_el->set_attribute(ctx_->strings.style, ss.str());
            
            // Update all rows for this column
            for (auto& row : rows_) {
                auto cell = row->get_cell(resizing_col_);
                if (cell) {
                    std::stringstream css;
                    css << "padding: 6px; box-sizing: border-box; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; user-select: none; ";
                    css << "width: " << new_w << "px; min-width: " << new_w << "px; max-width: " << new_w << "px;";
                    cell->set_attribute(ctx_->strings.style, css.str());
                }
            }
        }
    }
    
    void handle_mouseup(const Event& e) {
        resizing_col_ = -1;
    }

    std::shared_ptr<Element> header_container_;
    std::shared_ptr<Element> body_container_;
    std::vector<std::shared_ptr<GridRow<T>>> rows_;
    std::vector<ColumnDef> cols_;
    std::function<void(std::shared_ptr<GridRow<T>>, const std::string&)> on_cell_double_click_;
    std::function<void(std::shared_ptr<GridRow<T>>, const T&)> on_render_row_;
    
    int resizing_col_ = -1;
    double start_x_ = 0;
    int start_w_ = 0;
    int seq_counter_ = 0;
    
public:
    int sort_col_idx_ = -1;
    SortDirection sort_dir_ = SORT_NONE;
    std::function<bool(const T& a, const T& b, int col_idx)> sort_cmp_;
    
    void apply_sort() {
        for (int i = 0; i < cols_.size(); ++i) {
            if (cols_[i].sort_icon_el) {
                if (i == sort_col_idx_) {
                    cols_[i].sort_icon_el->set_text_content(ctx_->register_string(sort_dir_ == SORT_ASC ? "▲" : (sort_dir_ == SORT_DESC ? "▼" : "")));
                } else {
                    cols_[i].sort_icon_el->set_text_content(ctx_->register_string(""));
                }
            }
        }
        
        if (sort_col_idx_ < 0 || sort_dir_ == SORT_NONE || !sort_cmp_) {
            std::sort(rows_.begin(), rows_.end(), [](const std::shared_ptr<GridRow<T>>& a, const std::shared_ptr<GridRow<T>>& b) {
                return a->seq_ < b->seq_;
            });
        } else {
            std::sort(rows_.begin(), rows_.end(), [this](const std::shared_ptr<GridRow<T>>& a, const std::shared_ptr<GridRow<T>>& b) {
                if (sort_dir_ == SORT_ASC) return sort_cmp_(a->get_data(), b->get_data(), sort_col_idx_);
                return sort_cmp_(b->get_data(), a->get_data(), sort_col_idx_);
            });
        }
        
        body_container_->set_text_content(ctx_->register_string(""));
        for (auto& row : rows_) {
            body_container_->append_child(row);
        }
    }
private:
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
