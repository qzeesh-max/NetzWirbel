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
#include <string>

namespace NetzWirbel {

class Event {
public:
    static constexpr uint8_t MODIFIER_CTRL = 1 << 0;
    static constexpr uint8_t MODIFIER_SHIFT = 1 << 1;
    static constexpr uint8_t MODIFIER_ALT = 1 << 2;
    static constexpr uint8_t MODIFIER_META = 1 << 3;

    Event(const std::string& type) : type_(type) {}
    virtual ~Event() = default;

    const std::string& get_type() const { return type_; }

private:
    std::string type_;
};

class MouseEvent : public Event {
public:
    MouseEvent(const std::string& type, double client_x, double client_y, uint8_t modifiers = 0) 
        : Event(type), client_x_(client_x), client_y_(client_y), modifiers_(modifiers) {}

    double get_client_x() const { return client_x_; }
    double get_client_y() const { return client_y_; }
    
    bool ctrl_key() const { return (modifiers_ & MODIFIER_CTRL) != 0; }
    bool shift_key() const { return (modifiers_ & MODIFIER_SHIFT) != 0; }
    bool alt_key() const { return (modifiers_ & MODIFIER_ALT) != 0; }
    bool meta_key() const { return (modifiers_ & MODIFIER_META) != 0; }
    uint8_t get_modifiers() const { return modifiers_; }

private:
    double client_x_;
    double client_y_;
    uint8_t modifiers_;
};

class KeyboardEvent : public Event {
public:
    KeyboardEvent(const std::string& type, const std::string& key, uint8_t modifiers = 0) 
        : Event(type), key_(key), modifiers_(modifiers) {}

    const std::string& get_key() const { return key_; }

    bool ctrl_key() const { return (modifiers_ & MODIFIER_CTRL) != 0; }
    bool shift_key() const { return (modifiers_ & MODIFIER_SHIFT) != 0; }
    bool alt_key() const { return (modifiers_ & MODIFIER_ALT) != 0; }
    bool meta_key() const { return (modifiers_ & MODIFIER_META) != 0; }
    uint8_t get_modifiers() const { return modifiers_; }

private:
    std::string key_;
    uint8_t modifiers_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
