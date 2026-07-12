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
#include "NetzWirbel/RingBuffer.hpp"
#include "NetzWirbel/Command.hpp"
#include <vector>

using namespace NetzWirbel;

class RingBufferTest : public ::testing::Test {
protected:
    std::vector<uint8_t> mem;
    RingBufferHeader* header;
    RingBuffer* rb;

    void SetUp() override {
        mem.resize(1024);
        header = RingBuffer::init(mem.data(), 8, sizeof(Command));
        rb = new RingBuffer(header);
    }

    void TearDown() override {
        delete rb;
    }
};

TEST_F(RingBufferTest, BasicPushPop) {
    Command cmd1;
    cmd1.type = CommandType::CREATE_ELEMENT;
    cmd1.target_id = 1;
    cmd1.arg1 = 2;
    cmd1.arg2 = 3;

    EXPECT_TRUE(rb->push(&cmd1));

    Command cmd2;
    cmd2.type = CommandType::SET_ATTRIBUTE;
    cmd2.target_id = 1;

    EXPECT_TRUE(rb->push(&cmd2));

    Command out1;
    EXPECT_TRUE(rb->pop(&out1));
    EXPECT_EQ(out1.type, CommandType::CREATE_ELEMENT);
    EXPECT_EQ(out1.target_id, 1);
    EXPECT_EQ(out1.arg1, 2);
    EXPECT_EQ(out1.arg2, 3);

    Command out2;
    EXPECT_TRUE(rb->pop(&out2));
    EXPECT_EQ(out2.type, CommandType::SET_ATTRIBUTE);

    EXPECT_FALSE(rb->pop(&out1)); // Empty
}

TEST_F(RingBufferTest, WrapAround) {
    // Fill the buffer up to capacity - 1 (since 1 slot is always kept empty to distinguish full/empty)
    for (int i = 0; i < 7; ++i) {
        Command cmd;
        cmd.type = CommandType::CREATE_ELEMENT;
        cmd.target_id = i;
        EXPECT_TRUE(rb->push(&cmd));
    }
    
    // Should be full
    Command cmd_full;
    EXPECT_FALSE(rb->push(&cmd_full));

    // Pop 3 elements
    for (int i = 0; i < 3; ++i) {
        Command out;
        EXPECT_TRUE(rb->pop(&out));
        EXPECT_EQ(out.target_id, i);
    }

    // Push 2 elements, causing wrap around
    for (int i = 7; i < 9; ++i) {
        Command cmd;
        cmd.type = CommandType::SET_ATTRIBUTE;
        cmd.target_id = i;
        EXPECT_TRUE(rb->push(&cmd));
    }

    // Verify remaining
    for (int i = 3; i < 9; ++i) {
        Command out;
        EXPECT_TRUE(rb->pop(&out));
        EXPECT_EQ(out.target_id, i);
    }

    EXPECT_FALSE(rb->pop(&cmd_full));
}
