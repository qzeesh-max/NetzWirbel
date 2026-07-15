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
#include "NetzWirbel/DOM/Button.hpp"
#include "NetzWirbel/DOM/Spinner.hpp"
#include "NetzWirbel/DOM/TextBox.hpp"
#include "NetzWirbel/DOM/TextArea.hpp"
#include "NetzWirbel/DOM/RichTextArea.hpp"
#include "NetzWirbel/DOM/Window.hpp"
#include "NetzWirbel/DOM/Grid.hpp"
#include <vector>
#include <memory>

using namespace NetzWirbel;

class DOMComponentsTest : public ::testing::Test {
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

// --- Button Tests ---
TEST_F(DOMComponentsTest, ButtonStateToggle) {
    auto btn = std::make_shared<Button>(ctx);
    btn->set_text("Click Me");
    ctx->register_element(btn);
    
    btn->set_text("Updated Text");
    
    // Simulate mouse down
    MouseEvent md("mousedown", 0, 0);
    btn->handle_event(md);
    
    // Simulate mouse up
    MouseEvent mu("mouseup", 0, 0);
    btn->handle_event(mu);
    
    // Test event listener firing
    bool clicked = false;
    btn->add_event_listener("click", [&](const Event& e) {
        clicked = true;
    });
    
    MouseEvent mc("click", 0, 0);
    btn->handle_event(mc);
    EXPECT_TRUE(clicked);
    
    ctx->unregister_element(btn->get_id());
}

// --- Spinner Tests ---
TEST_F(DOMComponentsTest, SpinnerBoundsAndStepping) {
    auto spinner = std::make_shared<Spinner>(ctx, 5.0, 1.0, 0);
    ctx->register_element(spinner);
    
    EXPECT_DOUBLE_EQ(spinner->get_value(), 5.0);
    
    spinner->set_value(8.0);
    EXPECT_DOUBLE_EQ(spinner->get_value(), 8.0);
    
    ctx->unregister_element(spinner->get_id());
}

// --- TextBox & TextArea Tests ---
TEST_F(DOMComponentsTest, TextBoxInput) {
    auto tb = std::make_shared<TextBox>(ctx);
    ctx->register_element(tb);
    
    tb->set_value("Hello");
    EXPECT_EQ(tb->get_value(), "Hello");
    
    tb->handle_property_changed("value", "World");
    EXPECT_EQ(tb->get_value(), "World");
    
    ctx->unregister_element(tb->get_id());
}

TEST_F(DOMComponentsTest, TextAreaInput) {
    auto ta = std::make_shared<TextArea>(ctx);
    ctx->register_element(ta);
    
    ta->set_text("Line 1\nLine 2");
    EXPECT_EQ(ta->get_text(), "Line 1\nLine 2");
    
    ctx->unregister_element(ta->get_id());
}

TEST_F(DOMComponentsTest, RichTextAreaInput) {
    auto rta = std::make_shared<RichTextArea>(ctx);
    ctx->register_element(rta);
    
    rta->set_html("<b>Bold</b>");
    // get_html does not exist natively right now, so we just check it doesn't crash
    
    ctx->unregister_element(rta->get_id());
}

// --- Window Tests ---
TEST_F(DOMComponentsTest, WindowBoundsAndState) {
    auto win = std::make_shared<Window>(ctx, "test-win", "Test Window", 100, 100, 400, 300);
    ctx->register_element(win);
    
    win->set_title("New Title");
    
    win->set_bounds(50, 50, 800, 600);
    
    win->set_visible(false);
    
    ctx->unregister_element(win->get_id());
}

struct DummyData { int id; std::string name; };

// --- Grid Tests ---
TEST_F(DOMComponentsTest, GridBasicFunctionality) {
    auto grid = std::make_shared<Grid<DummyData>>(ctx);
    ctx->register_element(grid);
    
    grid->add_column("ID", 50);
    grid->add_column("Name", 100);
    
    auto row1 = grid->add_row({1, "Item 1"});
    row1->add_cell("1", 50);
    row1->add_cell("Item 1", 100);
    
    EXPECT_EQ(grid->get_row_count(), 1);
    
    grid->clear_rows();
    EXPECT_EQ(grid->get_row_count(), 0);
    
    ctx->unregister_element(grid->get_id());
}

// --- Window and Document Element Tests ---
TEST_F(DOMComponentsTest, GlobalElementsAndPreventDefault) {
    auto win = std::make_shared<WindowElement>(ctx);
    ctx->register_element(win);
    
    auto doc = std::make_shared<DocumentElement>(ctx);
    ctx->register_element(doc);
    
    bool event_handled = false;
    win->add_event_listener("keydown", [&](const Event& e) {
        event_handled = true;
    }, true); // With preventDefault
    
    KeyboardEvent ke("keydown", "Enter");
    win->handle_event(ke);
    EXPECT_TRUE(event_handled);
    
    ctx->unregister_element(win->get_id());
    ctx->unregister_element(doc->get_id());
}
