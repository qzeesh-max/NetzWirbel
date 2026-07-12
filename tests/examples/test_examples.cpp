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
#include "../../examples/Calculator/CalculatorApp.hpp"
#include "../../examples/CalculatorHtml/CalculatorHtmlApp.hpp"
#include "../../examples/MarketData/MarketDataApp.hpp"
#include "../../examples/MarketDataHtml/MarketDataHtmlApp.hpp"
#include "../../examples/CardGame/CardGameApp.hpp"
#include "../../examples/GalacticDisplay/GalacticDisplayApp.hpp"
#include "../../examples/BanzaiExchange/BanzaiExchangeApp.hpp"
#include "../../examples/BanzaiExchange/FixParser.cpp"
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
EM_JS(void, js_mock_websocket_examples, (), {
    if (!globalThis.WebSocket) {
        globalThis.WebSocket = class {
            constructor(url) {
                this.url = url;
                this.readyState = 1;
            }
            send(data) {}
            close() {}
        };
    }
});
#endif

using namespace NetzWirbel;

class ExamplesTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef __EMSCRIPTEN__
        js_mock_websocket_examples();
#endif
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

TEST_F(ExamplesTest, TestCalculatorAppInit) {
    CalculatorApp app;
    app.on_init(ctx.get());
    // If it didn't crash, the initialization is working.
}

TEST_F(ExamplesTest, TestCalculatorHtmlAppInit) {
    CalculatorHtmlApp app;
    app.on_init(ctx.get());
}

TEST_F(ExamplesTest, TestMarketDataAppInit) {
    MarketDataApp app;
    app.on_init(ctx.get());
}

TEST_F(ExamplesTest, TestMarketDataHtmlAppInit) {
    MarketDataHtmlApp app;
    app.on_init(ctx.get());
}

TEST_F(ExamplesTest, TestCardGameAppInit) {
    CardGameApp app;
    app.on_init(ctx.get());
}

TEST_F(ExamplesTest, TestGalacticDisplayAppInit) {
    GalacticDisplayApp app;
    app.on_init(ctx.get());
}

TEST_F(ExamplesTest, TestBanzaiExchangeAppInit) {
    BanzaiExchangeApp app;
    app.on_init(ctx.get());
}
