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

#include "NetzWirbel/DOM/Element.hpp"
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/Command.hpp"
#include <cstring>

namespace NetzWirbel {

uint32_t Element::next_id_ = 1; // 0 usually reserved for null/window/document

Element::Element(Context* ctx, const std::string& tag_name) 
    : ctx_(ctx), id_(next_id_++), tag_name_(tag_name) {
    
    // Allocate memory for the tag name to send to JS
    char* tag_ptr = new char[tag_name_.size() + 1];
    std::strcpy(tag_ptr, tag_name_.c_str());

    Command cmd;
    cmd.type = CommandType::CREATE_ELEMENT;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(tag_ptr);
    cmd.arg2 = static_cast<uint32_t>(tag_name_.size());
    
    ctx_->send_command(cmd);
}

Element::Element(Context* ctx, uint32_t tag_name_id) 
    : ctx_(ctx), id_(next_id_++), tag_name_("") {
    
    Command cmd;
    cmd.type = CommandType::CREATE_ELEMENT;
    cmd.target_id = id_;
    cmd.arg1 = tag_name_id;
    cmd.arg2 = 0xFFFFFFFF; // ID indicator
    
    ctx_->send_command(cmd);
}

Element::Element(Context* ctx, SkipCreate)
    : ctx_(ctx), id_(next_id_++), tag_name_("") {
}

Element::~Element() {
    // The element is either already unregistered from the context,
    // or the context is being destroyed. No need to unregister here.
}

void Element::set_attribute(const std::string& key, const std::string& value) {
    char* key_ptr = new char[key.size() + 1];
    std::strcpy(key_ptr, key.c_str());
    
    char* val_ptr = new char[value.size() + 1];
    std::strcpy(val_ptr, value.c_str());

    Command cmd;
    cmd.type = CommandType::SET_ATTRIBUTE;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(key_ptr);
    cmd.arg2 = static_cast<uint32_t>(key.size());
    cmd.arg3 = reinterpret_cast<uint32_t>(val_ptr);
    cmd.arg4 = static_cast<uint32_t>(value.size());

    ctx_->send_command(cmd);
}

void Element::set_attribute(uint32_t key_id, const std::string& value) {
    char* val_ptr = new char[value.size() + 1];
    std::strcpy(val_ptr, value.c_str());

    Command cmd;
    cmd.type = CommandType::SET_ATTRIBUTE;
    cmd.target_id = id_;
    cmd.arg1 = key_id;
    cmd.arg2 = 0xFFFFFFFF;
    cmd.arg3 = reinterpret_cast<uint32_t>(val_ptr);
    cmd.arg4 = static_cast<uint32_t>(value.size());

    ctx_->send_command(cmd);
}

void Element::set_attribute(uint32_t key_id, uint32_t value_id) {
    Command cmd;
    cmd.type = CommandType::SET_ATTRIBUTE;
    cmd.target_id = id_;
    cmd.arg1 = key_id;
    cmd.arg2 = 0xFFFFFFFF;
    cmd.arg3 = value_id;
    cmd.arg4 = 0xFFFFFFFF;

    ctx_->send_command(cmd);
}

void Element::set_style(const std::string& name, const std::string& value) {
    std::string css = name + ":" + value + ";";
    char* css_ptr = new char[css.size() + 1];
    std::strcpy(css_ptr, css.c_str());

    Command cmd;
    cmd.type = CommandType::SET_STYLES;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(css_ptr);
    cmd.arg2 = static_cast<uint32_t>(css.size());
    cmd.arg3 = 0;
    cmd.arg4 = 0;
    ctx_->send_command(cmd);
}

void Element::remove_style(const std::string& name) {
    std::string css = name + ":;";
    char* css_ptr = new char[css.size() + 1];
    std::strcpy(css_ptr, css.c_str());

    Command cmd;
    cmd.type = CommandType::SET_STYLES;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(css_ptr);
    cmd.arg2 = static_cast<uint32_t>(css.size());
    cmd.arg3 = 0;
    cmd.arg4 = 0;
    ctx_->send_command(cmd);
}

void Element::set_styles(std::initializer_list<std::pair<std::string, std::string>> styles) {
    std::string css;
    for (const auto& pair : styles) {
        css += pair.first + ":" + pair.second + ";";
    }
    
    char* css_ptr = new char[css.size() + 1];
    std::strcpy(css_ptr, css.c_str());

    Command cmd;
    cmd.type = CommandType::SET_STYLES;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(css_ptr);
    cmd.arg2 = static_cast<uint32_t>(css.size());
    cmd.arg3 = 0;
    cmd.arg4 = 0;
    ctx_->send_command(cmd);
}

void Element::set_property(const std::string& key, const std::string& value) {
    char* key_ptr = new char[key.size() + 1];
    std::strcpy(key_ptr, key.c_str());
    
    char* val_ptr = new char[value.size() + 1];
    std::strcpy(val_ptr, value.c_str());

    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_STRING;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(key_ptr);
    cmd.arg2 = static_cast<uint32_t>(key.size());
    cmd.arg3 = reinterpret_cast<uint32_t>(val_ptr);
    cmd.arg4 = static_cast<uint32_t>(value.size());

    ctx_->send_command(cmd);
}

void Element::set_property(uint32_t key_id, const std::string& value) {
    char* val_ptr = new char[value.size() + 1];
    std::strcpy(val_ptr, value.c_str());

    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_STRING;
    cmd.target_id = id_;
    cmd.arg1 = key_id;
    cmd.arg2 = 0xFFFFFFFF;
    cmd.arg3 = reinterpret_cast<uint32_t>(val_ptr);
    cmd.arg4 = static_cast<uint32_t>(value.size());

    ctx_->send_command(cmd);
}

void Element::set_property(uint32_t key_id, uint32_t value_id) {
    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_STRING;
    cmd.target_id = id_;
    cmd.arg1 = key_id;
    cmd.arg2 = 0xFFFFFFFF;
    cmd.arg3 = value_id;
    cmd.arg4 = 0xFFFFFFFF;

    ctx_->send_command(cmd);
}

void Element::set_property(const std::string& key, bool value) {
    char* key_ptr = new char[key.size() + 1];
    std::strcpy(key_ptr, key.c_str());

    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_BOOL;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(key_ptr);
    cmd.arg2 = static_cast<uint32_t>(key.size());
    cmd.arg3 = value ? 1 : 0;

    ctx_->send_command(cmd);
}

void Element::set_property(uint32_t key_id, bool value) {
    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_BOOL;
    cmd.target_id = id_;
    cmd.arg1 = key_id;
    cmd.arg2 = 0xFFFFFFFF;
    cmd.arg3 = value ? 1 : 0;

    ctx_->send_command(cmd);
}

void Element::set_property(const std::string& key, double value) {
    char* key_ptr = new char[key.size() + 1];
    std::strcpy(key_ptr, key.c_str());

    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_NUMBER;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(key_ptr);
    cmd.arg2 = static_cast<uint32_t>(key.size());
    cmd.num_val = value;

    ctx_->send_command(cmd);
}

void Element::set_property(uint32_t key_id, double value) {
    Command cmd;
    cmd.type = CommandType::SET_PROPERTY_NUMBER;
    cmd.target_id = id_;
    cmd.arg1 = key_id;
    cmd.arg2 = 0xFFFFFFFF;
    cmd.num_val = value;

    ctx_->send_command(cmd);
}

void Element::set_text_content_conflated(const std::string& text) {
    char* new_ptr = static_cast<char*>(std::malloc(text.size() + 1));
    if (new_ptr) std::strcpy(new_ptr, text.c_str());

    uint64_t new_val = (static_cast<uint64_t>(text.size()) << 32) | static_cast<uint32_t>(reinterpret_cast<uintptr_t>(new_ptr));
    uint64_t old_val = conflated_text_.ptr_and_len.exchange(new_val, std::memory_order_acq_rel);
    
    uint32_t old_ptr = static_cast<uint32_t>(old_val & 0xFFFFFFFF);
    if (old_ptr != 0) {
        std::free(reinterpret_cast<void*>(old_ptr));
    }

    uint32_t expected = 0;
    if (conflated_text_.queued.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
        Command cmd;
        cmd.type = CommandType::SET_TEXT_CONTENT_CONFLATED;
        cmd.target_id = id_;
        cmd.arg1 = reinterpret_cast<uint32_t>(&conflated_text_);
        ctx_->send_command(cmd);
    }
}

void Element::set_text_content(const std::string& text) {
    char* text_ptr = new char[text.size() + 1];
    std::strcpy(text_ptr, text.c_str());

    Command cmd;
    cmd.type = CommandType::SET_TEXT_CONTENT;
    cmd.target_id = id_;
    cmd.arg1 = reinterpret_cast<uint32_t>(text_ptr);
    cmd.arg2 = static_cast<uint32_t>(text.size());

    ctx_->send_command(cmd);
}

void Element::set_text_content(uint32_t text_id) {
    Command cmd;
    cmd.type = CommandType::SET_TEXT_CONTENT;
    cmd.target_id = id_;
    cmd.arg1 = text_id;
    cmd.arg2 = 0xFFFFFFFF;

    ctx_->send_command(cmd);
}

void Element::append_child(std::shared_ptr<Element> child) {
    children_.push_back(child);
    child->set_parent(this);

    Command cmd;
    cmd.type = CommandType::APPEND_CHILD;
    cmd.target_id = id_;
    cmd.arg1 = child->get_id();

    ctx_->send_command(cmd);
}

void Element::remove_child(std::shared_ptr<Element> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        children_.erase(it);
        child->set_parent(nullptr);
    }
}

void Element::focus() {
    Command cmd;
    cmd.type = CommandType::FOCUS;
    cmd.target_id = id_;
    ctx_->send_command(cmd);
}

void Element::select() {
    Command cmd;
    cmd.type = CommandType::SELECT;
    cmd.target_id = id_;
    ctx_->send_command(cmd);
}

void Element::add_event_listener(const std::string& event_type, std::function<void(const Event&)> callback, bool prevent_default) {
    bool is_first = event_listeners_[event_type].empty();
    event_listeners_[event_type].push_back(std::move(callback));

    // Only send the command to JS to add the listener if this is the first one for this type
    if (is_first) {
        char* type_ptr = new char[event_type.size() + 1];
        std::strcpy(type_ptr, event_type.c_str());

        Command cmd;
        cmd.type = CommandType::ADD_EVENT_LISTENER;
        cmd.target_id = id_;
        cmd.arg1 = reinterpret_cast<uint32_t>(type_ptr);
        cmd.arg2 = static_cast<uint32_t>(event_type.size());
        cmd.arg3 = prevent_default ? 1 : 0;

        ctx_->send_command(cmd);
    }
}

void Element::add_event_listener(uint32_t event_type_id, std::function<void(const Event&)> callback, bool prevent_default) {
    std::string internal_key = ctx_->get_string(event_type_id);
    if (internal_key.empty()) return; // Invalid ID

    bool is_first = event_listeners_[internal_key].empty();
    event_listeners_[internal_key].push_back(std::move(callback));

    if (is_first) {
        Command cmd;
        cmd.type = CommandType::ADD_EVENT_LISTENER;
        cmd.target_id = id_;
        cmd.arg1 = event_type_id;
        cmd.arg2 = 0xFFFFFFFF;
        cmd.arg3 = prevent_default ? 1 : 0;

        ctx_->send_command(cmd);
    }
}

void Element::handle_event(const Event& event) {
    auto it = event_listeners_.find(event.get_type());
    if (it != event_listeners_.end()) {
        for (const auto& listener : it->second) {
            listener(event);
        }
    }
    
    if (parent_) {
        parent_->handle_event(event);
    }
}

void Element::handle_property_changed(const std::string& prop_name, const std::string& value) {}
void Element::handle_property_changed(const std::string& prop_name, bool value) {}
void Element::handle_property_changed(const std::string& prop_name, double value) {}

} // namespace NetzWirbel
