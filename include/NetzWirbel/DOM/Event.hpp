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
    Event(const std::string& type) : type_(type) {}
    virtual ~Event() = default;

    const std::string& get_type() const { return type_; }

private:
    std::string type_;
};

class MouseEvent : public Event {
public:
    MouseEvent(const std::string& type, double client_x, double client_y) 
        : Event(type), client_x_(client_x), client_y_(client_y) {}

    double get_client_x() const { return client_x_; }
    double get_client_y() const { return client_y_; }

private:
    double client_x_;
    double client_y_;
};

class KeyboardEvent : public Event {
public:
    KeyboardEvent(const std::string& type, const std::string& key) 
        : Event(type), key_(key) {}

    const std::string& get_key() const { return key_; }

private:
    std::string key_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
