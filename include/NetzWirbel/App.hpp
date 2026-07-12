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
#include "NetzWirbel/Context.hpp"
#include <memory>

namespace NetzWirbel {

class App {
public:
    virtual ~App() = default;

    // Called once the NetzWirbelBridge connects and initializes the WebAssembly module
    virtual void on_init(Context* ctx) = 0;

    // Called every animation frame from JavaScript
    virtual void on_tick(double time) {}
};

// Global entry point. Call this in your main()
void run_app(std::unique_ptr<App> app);

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
