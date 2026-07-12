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

#include "NetzWirbel/Network.hpp"
#include <unordered_map>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#define EM_JS(ret, name, params, code) void name params {}
#endif

namespace NetzWirbel {

uint32_t WebSocket::next_id_ = 1;

static std::unordered_map<uint32_t, WebSocket*> g_sockets;

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void netzwirbel_ws_on_message(uint32_t id, const char* data, int len) {
        auto it = g_sockets.find(id);
        if (it != g_sockets.end()) {
            it->second->handle_message(std::string(data, len));
        }
    }

    EMSCRIPTEN_KEEPALIVE
    void netzwirbel_ws_on_close(uint32_t id) {
        auto it = g_sockets.find(id);
        if (it != g_sockets.end()) {
            it->second->handle_close();
        }
    }
}

#ifdef __EMSCRIPTEN__
EM_JS(void, js_connect_websocket, (uint32_t id, const char* url_ptr, int url_len), {
    if (!globalThis.netzwirbelSockets) {
        globalThis.netzwirbelSockets = {};
    }
    const url = UTF8ToString(url_ptr, url_len);
    const ws = new globalThis.WebSocket(url); // explicit globalThis.WebSocket to avoid conflict
    globalThis.netzwirbelSockets[id] = ws;

    ws.onmessage = function(event) {
        const text = event.data;
        const lengthBytes = lengthBytesUTF8(text) + 1;
        const stringOnWasmHeap = _malloc(lengthBytes);
        stringToUTF8(text, stringOnWasmHeap, lengthBytes);
        _netzwirbel_ws_on_message(id, stringOnWasmHeap, lengthBytes - 1);
        _free(stringOnWasmHeap);
    };

    ws.onclose = function(event) {
        _netzwirbel_ws_on_close(id);
        delete globalThis.netzwirbelSockets[id];
    };
});

EM_JS(void, js_send_websocket, (uint32_t id, const char* data_ptr, int data_len), {
    if (globalThis.netzwirbelSockets && globalThis.netzwirbelSockets[id]) {
        const ws = globalThis.netzwirbelSockets[id];
        if (ws.readyState === 1) { // 1 == OPEN
            const data = UTF8ToString(data_ptr, data_len);
            ws.send(data);
        }
    }
});

EM_JS(void, js_close_websocket, (uint32_t id), {
    if (globalThis.netzwirbelSockets && globalThis.netzwirbelSockets[id]) {
        globalThis.netzwirbelSockets[id].close();
        delete globalThis.netzwirbelSockets[id];
    }
});
#endif

WebSocket::WebSocket(Context* ctx, const std::string& url)
    : ctx_(ctx), id_(next_id_++), url_(url) {
    g_sockets[id_] = this;
    js_connect_websocket(id_, url.c_str(), url.size());
}

WebSocket::~WebSocket() {
    close();
    g_sockets.erase(id_);
}

void WebSocket::send(const std::string& data) {
    js_send_websocket(id_, data.c_str(), data.size());
}

void WebSocket::close() {
    js_close_websocket(id_);
}

} // namespace NetzWirbel
