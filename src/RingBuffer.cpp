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

#include "NetzWirbel/RingBuffer.hpp"
#include <cstring>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#else
// For non-emscripten compilation (e.g. native tests)
#include <thread>
#include <chrono>
#endif

namespace NetzWirbel {

size_t RingBuffer::calculate_size(uint32_t capacity, uint32_t item_size) {
    return sizeof(RingBufferHeader) + (capacity * item_size);
}

RingBufferHeader* RingBuffer::init(void* memory, uint32_t capacity, uint32_t item_size) {
    RingBufferHeader* header = static_cast<RingBufferHeader*>(memory);
    header->head.store(0, std::memory_order_relaxed);
    header->tail.store(0, std::memory_order_relaxed);
    header->capacity = capacity;
    header->item_size = item_size;
    header->state.store(0, std::memory_order_relaxed);
    return header;
}

RingBuffer::RingBuffer(RingBufferHeader* header) : header_(header) {
    buffer_data_ = reinterpret_cast<uint8_t*>(header_) + sizeof(RingBufferHeader);
}

bool RingBuffer::push(const void* item) {
    uint32_t head = header_->head.load(std::memory_order_relaxed);
    uint32_t tail = header_->tail.load(std::memory_order_acquire);
    uint32_t next_head = (head + 1) & (header_->capacity - 1);

    if (next_head == tail) {
        return false; // Buffer is full
    }

    uint8_t* dest = buffer_data_ + (head * header_->item_size);
    std::memcpy(dest, item, header_->item_size);

    header_->head.store(next_head, std::memory_order_release);
    return true;
}

bool RingBuffer::pop(void* item) {
    uint32_t tail = header_->tail.load(std::memory_order_relaxed);
    uint32_t head = header_->head.load(std::memory_order_acquire);

    if (tail == head) {
        return false; // Buffer is empty
    }

    uint8_t* src = buffer_data_ + (tail * header_->item_size);
    std::memcpy(item, src, header_->item_size);

    uint32_t next_tail = (tail + 1) & (header_->capacity - 1);
    header_->tail.store(next_tail, std::memory_order_release);
    return true;
}

bool RingBuffer::is_empty() const {
    return header_->head.load(std::memory_order_acquire) == header_->tail.load(std::memory_order_acquire);
}

bool RingBuffer::is_full() const {
    uint32_t head = header_->head.load(std::memory_order_acquire);
    uint32_t tail = header_->tail.load(std::memory_order_acquire);
    return ((head + 1) & (header_->capacity - 1)) == tail;
}

void* RingBuffer::get_buffer() const {
    return buffer_data_;
}

void RingBuffer::wait_until_not_empty() {
#ifdef __EMSCRIPTEN__
    while (is_empty()) {
        uint32_t expected_head = header_->head.load(std::memory_order_relaxed);
        // Wait on the head address until it changes from expected_head
        emscripten_futex_wait(reinterpret_cast<volatile void*>(&header_->head), expected_head, INFINITY);
    }
#else
    while (is_empty()) {
        std::this_thread::yield();
    }
#endif
}

void RingBuffer::notify() {
#ifdef __EMSCRIPTEN__
    emscripten_futex_wake(reinterpret_cast<volatile void*>(&header_->head), 1);
#else
    // Native no-op or condition variable
#endif
}

} // namespace NetzWirbel
