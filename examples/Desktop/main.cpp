/*
 * Copyright (C) 2026 NetzWirbel Contributors
 */

#include "DesktopApp.hpp"

int main() {
    NetzWirbel::run_app(std::make_unique<NetzWirbel::Examples::DesktopApp>());
    return 0;
}
