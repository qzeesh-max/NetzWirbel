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
#include "NetzWirbel/DOM/Event.hpp"
#include <vector>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <atomic>

namespace NetzWirbel {

class Context;

struct alignas(8) ConflatedText {
    std::atomic<uint64_t> ptr_and_len{0}; // high 32: length, low 32: pointer
    std::atomic<uint32_t> queued{0};
};

class Element : public std::enable_shared_from_this<Element> {
public:
    Element(Context* ctx, const std::string& tag_name);
    Element(Context* ctx, uint32_t tag_name_id);
    struct SkipCreate {};
    Element(Context* ctx, SkipCreate);
    virtual ~Element();

    uint32_t get_id() const { return id_; }
    const std::string& get_tag_name() const { return tag_name_; }
    
    Element* get_parent() const { return parent_; }
    void set_parent(Element* parent) { parent_ = parent; }

    void set_attribute(const std::string& key, const std::string& value);
    void set_attribute(uint32_t key_id, const std::string& value);
    void set_attribute(uint32_t key_id, uint32_t value_id);

    void set_style(const std::string& name, const std::string& value);
    void remove_style(const std::string& name);
    void set_styles(std::initializer_list<std::pair<std::string, std::string>> styles);

    void set_property(const std::string& key, const std::string& value);
    void set_property(uint32_t key_id, const std::string& value);
    void set_property(uint32_t key_id, uint32_t value_id);

    void set_property(const std::string& key, bool value);
    void set_property(uint32_t key_id, bool value);

    void set_property(const std::string& key, double value);
    void set_property(uint32_t key_id, double value);

    void set_text_content(const std::string& text);
    void set_text_content(uint32_t text_id);
    void set_text_content_conflated(const std::string& text);

    virtual void append_child(std::shared_ptr<Element> child);
    virtual void remove_child(std::shared_ptr<Element> child);

    void focus();
    void select();

    void add_event_listener(const std::string& event_type, std::function<void(const Event&)> callback, bool prevent_default = false);
    void add_event_listener(uint32_t event_type_id, std::function<void(const Event&)> callback, bool prevent_default = false);

    // Called by the Context when an event occurs
    virtual void handle_event(const Event& event);
    virtual void handle_property_changed(const std::string& prop_name, const std::string& value);
    virtual void handle_property_changed(const std::string& prop_name, bool value);
    virtual void handle_property_changed(const std::string& prop_name, double value);

protected:
    Context* ctx_;
    uint32_t id_;
    std::string tag_name_;
    std::string text_content_;
    
    ConflatedText conflated_text_;
    std::unordered_map<std::string, std::vector<std::function<void(const Event&)>>> event_listeners_;
    std::vector<std::shared_ptr<Element>> children_;
    Element* parent_ = nullptr;

private:
    static uint32_t next_id_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
