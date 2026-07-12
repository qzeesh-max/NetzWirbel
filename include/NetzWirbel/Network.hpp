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
#include <string>
#include <functional>
#include <memory>

namespace NetzWirbel {
class Context;

class WebSocket : public std::enable_shared_from_this<WebSocket> {
public:
    WebSocket(Context* ctx, const std::string& url);
    ~WebSocket();

    void send(const std::string& data);
    void close();

    void on_message(std::function<void(const std::string&)> callback) {
        on_message_cb_ = std::move(callback);
    }

    void on_close(std::function<void()> callback) {
        on_close_cb_ = std::move(callback);
    }

    // Called internally by the bridge
    void handle_message(const std::string& data) {
        if (on_message_cb_) on_message_cb_(data);
    }

    void handle_close() {
        if (on_close_cb_) on_close_cb_();
    }

    uint32_t get_id() const { return id_; }

private:
    Context* ctx_;
    uint32_t id_;
    std::string url_;
    std::function<void(const std::string&)> on_message_cb_;
    std::function<void()> on_close_cb_;

    static uint32_t next_id_;
};

// Deprecated global functions. We'll leave them here for backwards compatibility
// or just remove them if we refactored everything. But I'll remove them to ensure
// clean OOP.
namespace Network {
    // Left empty for now, maybe add HTTP features later
}

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
