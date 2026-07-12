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
#include <emscripten.h>
#include <cmath>

using namespace NetzWirbel;

class ContextTest : public ::testing::Test {
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

TEST_F(ContextTest, StatsMath) {
    RingBufferHeader* js_header = reinterpret_cast<RingBufferHeader*>(mem_js.data());
    RingBuffer js_rb(js_header);
    
    EventMsg msg;
    std::memset(&msg, 0, sizeof(EventMsg));
    msg.type = EventType::PONG;
    
    double values[] = {10, 20, 20, 20, 30, 40, 50};
    for (int i = 0; i < 7; ++i) {
        msg.num_val = emscripten_get_now() - values[i];
        EXPECT_TRUE(js_rb.push(&msg));
        ctx->process_events(); 
    }

    auto stats = ctx->get_rtt_stats();
    // Verify stats values roughly due to time processing delays
    EXPECT_LT(std::abs(stats.median - 20.0), 2.0);
    EXPECT_LT(std::abs(stats.low - 10.0), 2.0);
    EXPECT_LT(std::abs(stats.high - 50.0), 2.0);
}

TEST_F(ContextTest, RegisterUnregisterElement) {
    auto input = std::make_shared<HTMLInputElement>(ctx);
    ctx->register_element(input);
    
    // Unregister should not crash
    ctx->unregister_element(input->get_id());
}

TEST_F(ContextTest, RegisterString) {
    uint32_t id1 = ctx->register_string("test_string_1");
    uint32_t id2 = ctx->register_string("test_string_2");
    uint32_t id3 = ctx->register_string("test_string_1"); // Duplicate should return same ID

    EXPECT_NE(id1, id2);
    EXPECT_EQ(id1, id3);

    EXPECT_EQ(ctx->get_string(id1), "test_string_1");
    EXPECT_EQ(ctx->get_string(id2), "test_string_2");
    EXPECT_EQ(ctx->get_string(9999), ""); // Invalid ID returns empty

    // Verify common strings are registered
    EXPECT_GT(ctx->strings.div, 0);
    EXPECT_EQ(ctx->get_string(ctx->strings.div), "div");
    EXPECT_EQ(ctx->get_string(ctx->strings.color), "color");
    EXPECT_EQ(ctx->get_string(ctx->strings.true_), "true");
    EXPECT_EQ(ctx->get_string(ctx->strings.text), "text");
}

TEST_F(ContextTest, CommandBacklogOverflow) {
    uint32_t capacity = 128;
    size_t mem_size_cpp = RingBuffer::calculate_size(capacity, sizeof(Command));
    size_t mem_size_js = RingBuffer::calculate_size(capacity, sizeof(EventMsg));
    std::vector<uint8_t> small_mem_cpp(mem_size_cpp);
    std::vector<uint8_t> small_mem_js(mem_size_js);
    Context small_ctx(small_mem_cpp.data(), small_mem_js.data(), capacity);

    RingBufferHeader* header = reinterpret_cast<RingBufferHeader*>(small_mem_cpp.data());
    RingBuffer rb(header);
    Command trash;
    while (rb.pop(&trash)) {} // Clear all string registrations from constructor

    int num_to_push = capacity + 10;
    for (int i = 0; i < num_to_push; ++i) {
        Command cmd;
        cmd.type = CommandType::CREATE_ELEMENT;
        cmd.target_id = 1000 + i;
        small_ctx.send_command(cmd);
    }

    for (int i = 0; i < (int)capacity - 1; ++i) {
        Command out;
        EXPECT_TRUE(rb.pop(&out));
        EXPECT_EQ(out.target_id, 1000 + i);
    }

    small_ctx.flush_command_backlog();

    for (int i = (int)capacity - 1; i < num_to_push; ++i) {
        Command out;
        EXPECT_TRUE(rb.pop(&out));
        EXPECT_EQ(out.target_id, 1000 + i);
    }
}
