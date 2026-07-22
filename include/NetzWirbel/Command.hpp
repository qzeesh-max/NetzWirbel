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
#include <cstdint>

namespace NetzWirbel {

enum class CommandType : uint32_t {
    CREATE_ELEMENT = 1,
    SET_ATTRIBUTE = 2,
    SET_PROPERTY_STRING = 3,
    SET_PROPERTY_BOOL = 4,
    SET_PROPERTY_NUMBER = 5,
    ADD_EVENT_LISTENER = 6,
    APPEND_CHILD = 7,
    SET_TEXT_CONTENT = 8,
    PING = 9,
    BIND_ELEMENT = 10,
    REGISTER_STRING = 11,
    FOCUS = 12,
    SELECT = 13,
    SET_NUMERIC_ONLY = 14,
    SET_STYLES = 15,
    SET_TEXT_CONTENT_CONFLATED = 16,
    SET_CLASS_CONFLATED = 17,
    SET_STYLE_CONFLATED = 18,
    REMOVE_CHILD = 19,
    DESTROY_ELEMENT = 20
};

// Represents a command from C++ to JS
struct Command {
    CommandType type;
    uint32_t target_id;
    uint32_t arg1; // ID, or pointer
    uint32_t arg2; // ID, or length, or boolean value
    uint32_t arg3; // pointer
    uint32_t arg4; // length
    double num_val;
};

enum class EventType : uint32_t {
    EVENT = 1,
    PROPERTY_CHANGED_STRING = 2,
    PROPERTY_CHANGED_BOOL = 3,
    PROPERTY_CHANGED_NUMBER = 4,
    PONG = 5
};

// Represents an event from JS to C++
struct EventMsg {
    EventType type;
    uint32_t target_id;
    uint32_t event_type_ptr; // pointer to event type string (e.g. "click")
    uint32_t event_type_len;
    uint32_t prop_name_ptr;
    uint32_t prop_name_len;
    uint32_t str_val_ptr;
    uint32_t str_val_len;
    double num_val;
    bool bool_val;
    uint8_t modifiers; // Bitmask: 1=Ctrl, 2=Shift, 4=Alt, 8=Meta
    double client_x;
    double client_y;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
