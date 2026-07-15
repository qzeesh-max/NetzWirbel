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
#include "NetzWirbel/Network.hpp"
#include "NetzWirbel/Context.hpp"
#include <memory>

using namespace NetzWirbel;

class NetworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        cpp_to_js_mem = std::malloc(4096 * 32 + 24);
        js_to_cpp_mem = std::malloc(4096 * 64 + 24);
        ctx = std::make_unique<Context>(cpp_to_js_mem, js_to_cpp_mem, 4096);
    }

    void TearDown() override {
        ctx.reset();
        std::free(cpp_to_js_mem);
        std::free(js_to_cpp_mem);
    }

    void* cpp_to_js_mem;
    void* js_to_cpp_mem;
    std::unique_ptr<Context> ctx;
};

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, js_mock_websocket, (), {
    globalThis.WebSocket = class {
        constructor(url) {
            this.url = url;
            this.readyState = 1;
        }
        send(data) {}
        close() {}
    };
});
#endif

TEST_F(NetworkTest, TestWebSocketMessageCallback) {
#ifdef __EMSCRIPTEN__
    js_mock_websocket();
#endif
    auto ws = std::make_shared<WebSocket>(ctx.get(), "ws://localhost:3000");

    bool callback_called = false;
    std::string received_msg;

    ws->on_message([&](const std::string& msg) {
        callback_called = true;
        received_msg = msg;
    });

    // Simulate receiving a message from JS bridge
    ws->handle_message("Hello from JS");

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_msg, "Hello from JS");
}

TEST_F(NetworkTest, TestWebSocketCloseCallback) {
    auto ws = std::make_shared<WebSocket>(ctx.get(), "ws://localhost:3000");

    bool callback_called = false;

    ws->on_close([&]() {
        callback_called = true;
    });

    // Simulate receiving close event
    ws->handle_close();

    EXPECT_TRUE(callback_called);
}

TEST_F(NetworkTest, TestWebSocketSend) {
    auto ws = std::make_shared<WebSocket>(ctx.get(), "ws://localhost:3000");
    // Just checking it doesn't crash in mock environment (emscripten headers mock JS)
    ws->send("Test Data");
}
