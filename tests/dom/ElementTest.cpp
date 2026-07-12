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

#include <gtest/gtest.h>
#include "NetzWirbel/Context.hpp"
#include "NetzWirbel/DOM/Elements.hpp"
#include <vector>

using namespace NetzWirbel;

class ElementTest : public ::testing::Test {
protected:
    std::vector<uint8_t> mem_cpp;
    std::vector<uint8_t> mem_js;
    Context* ctx;

    void SetUp() override {
        mem_cpp.resize(10240);
        mem_js.resize(10240);
        ctx = new Context(mem_cpp.data(), mem_js.data(), 1024);
    }

    void TearDown() override {
        delete ctx;
    }
};

TEST_F(ElementTest, HTMLInputElement) {
    auto input = std::make_shared<HTMLInputElement>(ctx);
    ctx->register_element(input);

    EXPECT_EQ(input->get_value(), "");

    input->set_value("initial_val");
    EXPECT_EQ(input->get_value(), "initial_val");

    // Simulate JS updating the property
    input->handle_property_changed("value", "changed_from_js");
    EXPECT_EQ(input->get_value(), "changed_from_js");

    ctx->unregister_element(input->get_id());
}

TEST_F(ElementTest, HTMLImageElement) {
    auto img = std::make_shared<HTMLImageElement>(ctx);
    ctx->register_element(img);
    
    EXPECT_EQ(img->get_src(), "");
    img->set_src("test.png");
    EXPECT_EQ(img->get_src(), "test.png");

    ctx->unregister_element(img->get_id());
}

TEST_F(ElementTest, UnknownPropertyChange) {
    auto input = std::make_shared<HTMLInputElement>(ctx);
    ctx->register_element(input);

    // Ensure it doesn't crash on unknown property
    input->handle_property_changed("unknown_prop", "some_value");

    ctx->unregister_element(input->get_id());
}

TEST_F(ElementTest, OverloadedIDMethods) {
    uint32_t div_id = ctx->strings.div;
    Element el(ctx, div_id);
    
    uint32_t class_id = ctx->strings.class_;
    uint32_t val_id = ctx->register_string("container");
    
    // Use ID overrides
    el.set_attribute(class_id, val_id);
    el.set_property(ctx->strings.id, ctx->register_string("my-div"));
    el.set_text_content(ctx->register_string("Hello World"));
    
    // Add event listener via ID
    bool event_triggered = false;
    el.add_event_listener(ctx->strings.click, [&](const Event& e) {
        event_triggered = true;
    });
    
    Event e("click");
    el.handle_event(e);
    
    EXPECT_TRUE(event_triggered);
}
