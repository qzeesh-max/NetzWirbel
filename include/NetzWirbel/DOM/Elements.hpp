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
#include "NetzWirbel/Context.hpp"
#include <string>

namespace NetzWirbel {

class HTMLDivElement : public Element {
public:
    HTMLDivElement(Context* ctx) : Element(ctx, ctx->strings.div) {}
};

class HTMLButtonElement : public Element {
public:
    HTMLButtonElement(Context* ctx) : Element(ctx, ctx->strings.button) {}

    bool get_disabled() const { return disabled_; }
    void set_disabled(bool disabled) {
        disabled_ = disabled;
        set_property(ctx_->strings.disabled, disabled);
    }
    
    void handle_property_changed(const std::string& prop_name, bool value) override {
        if (prop_name == "disabled") disabled_ = value;
        else Element::handle_property_changed(prop_name, value);
    }

private:
    bool disabled_ = false;
};

class HTMLInputElement : public Element {
public:
    HTMLInputElement(Context* ctx) : Element(ctx, ctx->strings.input) {}

    std::string get_value() const { return value_; }
    void set_value(const std::string& value) {
        value_ = value;
        set_property(ctx_->strings.value, value);
    }

    bool get_disabled() const { return disabled_; }
    void set_disabled(bool disabled) {
        disabled_ = disabled;
        set_property(ctx_->strings.disabled, disabled);
    }

    void set_numeric_only(bool allow_decimal = false) {
        Command cmd;
        cmd.type = CommandType::SET_NUMERIC_ONLY;
        cmd.target_id = id_;
        cmd.arg1 = allow_decimal ? 1 : 0;
        ctx_->send_command(cmd);
    }

    void handle_property_changed(const std::string& prop_name, const std::string& value) override {
        if (prop_name == "value") value_ = value;
        else Element::handle_property_changed(prop_name, value);
    }

private:
    std::string value_ = "";
    bool disabled_ = false;
};

class HTMLSelectElement : public Element {
public:
    HTMLSelectElement(Context* ctx) : Element(ctx, ctx->strings.select) {}

    std::string get_value() const { return value_; }
    void set_value(const std::string& value) {
        value_ = value;
        set_property(ctx_->strings.value, value);
    }

    void handle_property_changed(const std::string& prop_name, const std::string& value) override {
        if (prop_name == "value") value_ = value;
        else Element::handle_property_changed(prop_name, value);
    }

private:
    std::string value_ = "";
};

class HTMLOptionElement : public Element {
public:
    HTMLOptionElement(Context* ctx) : Element(ctx, ctx->strings.option) {}
};

class HTMLCanvasElement : public Element {
public:
    HTMLCanvasElement(Context* ctx) : Element(ctx, ctx->register_string("canvas")) {}

    int get_width() const { return width_; }
    void set_width(int width) {
        width_ = width;
        set_property(ctx_->register_string("width"), static_cast<double>(width));
    }

    int get_height() const { return height_; }
    void set_height(int height) {
        height_ = height;
        set_property(ctx_->register_string("height"), static_cast<double>(height));
    }

    void handle_property_changed(const std::string& prop_name, double value) override {
        if (prop_name == "width") width_ = static_cast<int>(value);
        else if (prop_name == "height") height_ = static_cast<int>(value);
        else Element::handle_property_changed(prop_name, value);
    }

private:
    int width_ = 300;
    int height_ = 150;
};

class HTMLImageElement : public Element {
public:
    HTMLImageElement(Context* ctx) : Element(ctx, ctx->strings.img) {}

    std::string get_src() const { return src_; }
    void set_src(const std::string& src) {
        src_ = src;
        set_attribute(ctx_->strings.src, src);
    }

private:
    std::string src_ = "";
};

class WindowElement : public Element {
public:
    WindowElement(Context* ctx) : Element(ctx, SkipCreate{}) {}
};

class DocumentElement : public Element {
public:
    DocumentElement(Context* ctx) : Element(ctx, SkipCreate{}) {}
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
