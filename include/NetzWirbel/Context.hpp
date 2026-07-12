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
#include "NetzWirbel/RingBuffer.hpp"
#include "NetzWirbel/Command.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <deque>
#include <vector>

namespace NetzWirbel {

class Element;

class Context {
public:
    Context(void* cpp_to_js_mem, void* js_to_cpp_mem, uint32_t capacity);

    void register_element(std::shared_ptr<Element> el);
    void unregister_element(uint32_t id);
    std::shared_ptr<Element> get_element(uint32_t id);
    void bind_element(std::shared_ptr<Element> el, const std::string& selector);

    void send_command(const Command& cmd);
    void process_events();
    void flush_command_backlog();

    uint32_t register_string(const std::string& str);
    std::string get_string(uint32_t id) const;

    struct CommonStrings {
        // Elements
        uint32_t div, span, input, button, a, img, p, h1, h2, h3, h4, h5, h6;
        uint32_t table, tr, td, th, thead, tbody, form, label, select, option, textarea, ul, li;
        
        // Attributes
        uint32_t id, class_, style, src, href, type, value, placeholder, disabled, checked, name, data_val;
        
        // Styles
        uint32_t color, background_color, font_size, font_weight, margin, padding, border, display, flex, grid;
        
        // Events
        uint32_t click, mouseup, mousedown, mousemove, input_event, change, keydown, keyup, submit;

        // Common Values
        uint32_t true_, false_, text, zero;
    } strings;

    struct RTTStats {
        double low = 0;
        double high = 0;
        double avg = 0;
        double mode = 0;
        double median = 0;
        double stddev = 0;
        double p99 = 0;
    };

    void send_ping();
    RTTStats get_rtt_stats() const;

private:
    void init_common_strings();

    RingBuffer cpp_to_js_;
    RingBuffer js_to_cpp_;
    std::deque<Command> command_backlog_;
    std::unordered_map<uint32_t, std::shared_ptr<Element>> elements_;
    std::deque<double> rtt_history_;
    size_t max_history_ = 1000;

    std::unordered_map<std::string, uint32_t> string_to_id_;
    std::unordered_map<uint32_t, std::string> id_to_string_;
    uint32_t next_string_id_ = 1;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
