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

#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Element.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <emscripten.h>

namespace NetzWirbel {

Context::Context(void* cpp_to_js_mem, void* js_to_cpp_mem, uint32_t capacity)
    : cpp_to_js_(RingBuffer::init(cpp_to_js_mem, capacity, sizeof(Command))),
      js_to_cpp_(RingBuffer::init(js_to_cpp_mem, capacity, sizeof(EventMsg))) {
    init_common_strings();
}

uint32_t Context::register_string(const std::string& str) {
    auto it = string_to_id_.find(str);
    if (it != string_to_id_.end()) {
        return it->second;
    }

    uint32_t id = next_string_id_++;
    string_to_id_[str] = id;
    id_to_string_[id] = str;

    char* ptr = new char[str.size()];
    std::memcpy(ptr, str.data(), str.size());

    Command cmd;
    cmd.type = CommandType::REGISTER_STRING;
    cmd.target_id = id;
    cmd.arg1 = reinterpret_cast<uint32_t>(ptr);
    cmd.arg2 = static_cast<uint32_t>(str.size());
    send_command(cmd);

    return id;
}

std::string Context::get_string(uint32_t id) const {
    auto it = id_to_string_.find(id);
    if (it != id_to_string_.end()) {
        return it->second;
    }
    return "";
}

void Context::init_common_strings() {
    strings.div = register_string("div");
    strings.span = register_string("span");
    strings.input = register_string("input");
    strings.button = register_string("button");
    strings.a = register_string("a");
    strings.img = register_string("img");
    strings.p = register_string("p");
    strings.h1 = register_string("h1");
    strings.h2 = register_string("h2");
    strings.h3 = register_string("h3");
    strings.h4 = register_string("h4");
    strings.h5 = register_string("h5");
    strings.h6 = register_string("h6");
    strings.table = register_string("table");
    strings.tr = register_string("tr");
    strings.td = register_string("td");
    strings.th = register_string("th");
    strings.thead = register_string("thead");
    strings.tbody = register_string("tbody");
    strings.form = register_string("form");
    strings.label = register_string("label");
    strings.select = register_string("select");
    strings.option = register_string("option");
    strings.textarea = register_string("textarea");
    strings.ul = register_string("ul");
    strings.li = register_string("li");

    strings.id = register_string("id");
    strings.class_ = register_string("class");
    strings.style = register_string("style");
    strings.src = register_string("src");
    strings.href = register_string("href");
    strings.type = register_string("type");
    strings.value = register_string("value");
    strings.placeholder = register_string("placeholder");
    strings.disabled = register_string("disabled");
    strings.checked = register_string("checked");
    strings.name = register_string("name");
    strings.data_val = register_string("data-val");

    strings.color = register_string("color");
    strings.background_color = register_string("background-color");
    strings.font_size = register_string("font-size");
    strings.font_weight = register_string("font-weight");
    strings.margin = register_string("margin");
    strings.padding = register_string("padding");
    strings.border = register_string("border");
    strings.display = register_string("display");
    strings.flex = register_string("flex");
    strings.grid = register_string("grid");

    strings.click = register_string("click");
    strings.mouseup = register_string("mouseup");
    strings.mousedown = register_string("mousedown");
    strings.mousemove = register_string("mousemove");
    strings.input_event = register_string("input");
    strings.change = register_string("change");
    strings.keydown = register_string("keydown");
    strings.keyup = register_string("keyup");
    strings.submit = register_string("submit");

    strings.true_ = register_string("true");
    strings.false_ = register_string("false");
    strings.text = register_string("text");
    strings.zero = register_string("0");
}

void Context::register_element(std::shared_ptr<Element> el) {
    elements_[el->get_id()] = el;
}

void Context::unregister_element(uint32_t id) {
    elements_.erase(id);
}

std::shared_ptr<Element> Context::get_element(uint32_t id) {
    auto it = elements_.find(id);
    if (it != elements_.end()) {
        return it->second;
    }
    return nullptr;
}

void Context::bind_element(std::shared_ptr<Element> el, const std::string& selector) {
    char* selector_ptr = new char[selector.size() + 1];
    std::strcpy(selector_ptr, selector.c_str());

    Command cmd;
    cmd.type = CommandType::BIND_ELEMENT;
    cmd.target_id = el->get_id();
    cmd.arg1 = reinterpret_cast<uint32_t>(selector_ptr);
    cmd.arg2 = static_cast<uint32_t>(selector.size());

    send_command(cmd);
}

void Context::send_command(const Command& cmd) {
    if (!command_backlog_.empty()) {
        command_backlog_.push_back(cmd);
        flush_command_backlog();
        return;
    }

    if (!cpp_to_js_.push(&cmd)) {
        command_backlog_.push_back(cmd);
    } else {
        cpp_to_js_.notify();
    }
}

void Context::flush_command_backlog() {
    while (!command_backlog_.empty()) {
        const Command& cmd = command_backlog_.front();
        if (cpp_to_js_.push(&cmd)) {
            command_backlog_.pop_front();
        } else {
            break;
        }
    }
    cpp_to_js_.notify();
}

void Context::send_ping() {
    Command cmd{};
    cmd.type = CommandType::PING;
    cmd.num_val = emscripten_get_now(); // ms
    send_command(cmd);
}

Context::RTTStats Context::get_rtt_stats() const {
    RTTStats stats;
    if (rtt_history_.empty()) return stats;

    std::vector<double> sorted(rtt_history_.begin(), rtt_history_.end());
    std::sort(sorted.begin(), sorted.end());

    stats.low = sorted.front();
    stats.high = sorted.back();

    double sum = 0;
    for (double v : sorted) sum += v;
    stats.avg = sum / sorted.size();

    stats.median = sorted[sorted.size() / 2];
    stats.p99 = sorted[(size_t)(sorted.size() * 0.99)];

    double variance = 0;
    for (double v : sorted) variance += (v - stats.avg) * (v - stats.avg);
    stats.stddev = std::sqrt(variance / sorted.size());

    std::unordered_map<int, int> freq;
    for (double v : sorted) {
        freq[(int)(v * 10)]++; // bin to 0.1ms
    }
    int max_freq = 0;
    int mode_bin = 0;
    for (auto& p : freq) {
        if (p.second > max_freq) {
            max_freq = p.second;
            mode_bin = p.first;
        }
    }
    stats.mode = mode_bin / 10.0;

    return stats;
}

void Context::process_events() {
    EventMsg msg;
    while (js_to_cpp_.pop(&msg)) {
        if (msg.type == EventType::PONG) {
            double rtt = emscripten_get_now() - msg.num_val;
            rtt_history_.push_back(rtt);
            if (rtt_history_.size() > max_history_) {
                rtt_history_.pop_front();
            }
            continue;
        }

        auto el = get_element(msg.target_id);
        if (!el) {
            // Memory cleanup for unhandled events
            if (msg.event_type_ptr) delete[] reinterpret_cast<char*>(msg.event_type_ptr);
            if (msg.prop_name_ptr) delete[] reinterpret_cast<char*>(msg.prop_name_ptr);
            if (msg.str_val_ptr) delete[] reinterpret_cast<char*>(msg.str_val_ptr);
            continue;
        }

        switch (msg.type) {
            case EventType::EVENT: {
                if (msg.event_type_ptr) {
                    std::string event_type(reinterpret_cast<const char*>(msg.event_type_ptr), msg.event_type_len);
                    if (event_type == "mousemove" || event_type == "mousedown" || event_type == "mouseup" || event_type == "click") {
                        MouseEvent ev(event_type, msg.client_x, msg.client_y);
                        el->handle_event(ev);
                    } else if (event_type == "keydown" || event_type == "keyup") {
                        std::string key_str = "";
                        if (msg.str_val_ptr) {
                            key_str = std::string(reinterpret_cast<const char*>(msg.str_val_ptr), msg.str_val_len);
                            delete[] reinterpret_cast<char*>(msg.str_val_ptr);
                        }
                        KeyboardEvent ev(event_type, key_str);
                        el->handle_event(ev);
                    } else {
                        Event ev(event_type);
                        el->handle_event(ev);
                    }
                    delete[] reinterpret_cast<char*>(msg.event_type_ptr);
                }
                break;
            }
            case EventType::PROPERTY_CHANGED_STRING: {
                if (msg.prop_name_ptr && msg.str_val_ptr) {
                    std::string prop_name(reinterpret_cast<const char*>(msg.prop_name_ptr), msg.prop_name_len);
                    std::string str_val(reinterpret_cast<const char*>(msg.str_val_ptr), msg.str_val_len);
                    el->handle_property_changed(prop_name, str_val);
                    delete[] reinterpret_cast<char*>(msg.prop_name_ptr);
                    delete[] reinterpret_cast<char*>(msg.str_val_ptr);
                }
                break;
            }
            case EventType::PROPERTY_CHANGED_BOOL: {
                if (msg.prop_name_ptr) {
                    std::string prop_name(reinterpret_cast<const char*>(msg.prop_name_ptr), msg.prop_name_len);
                    el->handle_property_changed(prop_name, msg.bool_val);
                    delete[] reinterpret_cast<char*>(msg.prop_name_ptr);
                }
                break;
            }
            case EventType::PROPERTY_CHANGED_NUMBER: {
                if (msg.prop_name_ptr) {
                    std::string prop_name(reinterpret_cast<const char*>(msg.prop_name_ptr), msg.prop_name_len);
                    el->handle_property_changed(prop_name, msg.num_val);
                    delete[] reinterpret_cast<char*>(msg.prop_name_ptr);
                }
                break;
            }
        }
    }
}

} // namespace NetzWirbel
