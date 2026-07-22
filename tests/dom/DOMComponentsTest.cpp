/*
 * Copyright (C) 2026 NetzWirbel Contributors
 */

#include "NetzWirbel/DOM/Menu.hpp"
#include "NetzWirbel/DOM/Toolbar.hpp"
#include "NetzWirbel/DOM/StatusBar.hpp"
#include "NetzWirbel/DOM/TabContainer.hpp"
#include "NetzWirbel/Context.hpp"
#include <cassert>
#include <iostream>

using namespace NetzWirbel;

#include <gtest/gtest.h>

TEST(DOMComponentsTest, BasicTest) {
    std::vector<uint8_t> mem_cpp(10240);
    std::vector<uint8_t> mem_js(10240);
    Context ctx_obj(mem_cpp.data(), mem_js.data(), 1024);
    Context* ctx = &ctx_obj;

    // Menu tests
    auto menu = std::make_shared<Menu>(ctx);
    auto item = std::make_shared<MenuItem>(ctx, "Test", "icon", "Ctrl+T", false);
    menu->add_item(item);
    
    // Toolbar tests
    ToolbarConfig config;
    config.buttons.push_back({"icon", "tooltip", nullptr});
    auto toolbar = Toolbar::createFromTemplate(ctx, config);

    // StatusBar tests
    auto status = std::make_shared<StatusBar>(ctx);
    auto panel = std::make_shared<StatusBarPanel>(ctx, BevelStyle::Embossed);
    status->add_panel(panel);

    // Tab tests
    auto tabs = std::make_shared<TabContainer>(ctx);
    auto content = std::make_shared<HTMLDivElement>(ctx);
    tabs->add_tab("Tab 1", content);
    EXPECT_EQ(tabs->get_active_index(), 0);
}
