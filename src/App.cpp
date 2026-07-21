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

#include "NetzWirbel/App.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <iostream>

namespace NetzWirbel {

static std::unique_ptr<App> g_app_instance;
static Context* g_ctx_instance = nullptr;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void netzwirbel_init(void* cpp_to_js_mem, void* js_to_cpp_mem, uint32_t capacity) {
    if (g_ctx_instance) {
        delete g_ctx_instance;
    }
    g_ctx_instance = new Context(cpp_to_js_mem, js_to_cpp_mem, capacity);
    if (g_app_instance) {
        g_app_instance->on_init(g_ctx_instance);
    }
}

EMSCRIPTEN_KEEPALIVE
void netzwirbel_tick(double time) {
    if (g_ctx_instance) {
        g_ctx_instance->flush_command_backlog();
        g_ctx_instance->process_events();
    }
    if (g_app_instance) {
        g_app_instance->on_tick(time);
    }
    if (g_ctx_instance) {
        g_ctx_instance->flush_command_backlog();
    }
}

EMSCRIPTEN_KEEPALIVE
size_t netzwirbel_get_ring_buffer_header_size() {
    return sizeof(NetzWirbel::RingBufferHeader);
}

EMSCRIPTEN_KEEPALIVE
size_t netzwirbel_get_command_size() {
    return sizeof(NetzWirbel::Command);
}

EMSCRIPTEN_KEEPALIVE
size_t netzwirbel_get_event_msg_size() {
    return sizeof(NetzWirbel::EventMsg);
}

EMSCRIPTEN_KEEPALIVE
size_t netzwirbel_get_ring_buffer_head_offset() {
    return offsetof(NetzWirbel::RingBufferHeader, head);
}

EMSCRIPTEN_KEEPALIVE
size_t netzwirbel_get_ring_buffer_tail_offset() {
    return offsetof(NetzWirbel::RingBufferHeader, tail);
}

} // extern "C"

void run_app(std::unique_ptr<App> app) {
    g_app_instance = std::move(app);
    if (g_ctx_instance) {
        g_app_instance->on_init(g_ctx_instance);
    }
#ifdef __EMSCRIPTEN__
    emscripten_exit_with_live_runtime();
#endif
}

} // namespace NetzWirbel
