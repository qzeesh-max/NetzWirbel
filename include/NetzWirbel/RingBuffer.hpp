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

#include <atomic>
#include <cstdint>
#include <cstddef>

namespace NetzWirbel {

// A simple header for the ring buffer that sits at the start of the allocated memory.
// We align it to 8 bytes to avoid unaligned access penalties.
struct alignas(8) RingBufferHeader {
    std::atomic<uint32_t> head; // Index where producer writes
    std::atomic<uint32_t> tail; // Index where consumer reads
    uint32_t capacity;          // Must be a power of 2
    uint32_t item_size;         // Size of each item in bytes
    std::atomic<uint32_t> state; // General purpose state flag for synchronization
};

// Represents the lock-free ring buffer for WebAssembly SharedArrayBuffer
class RingBuffer {
public:
    // Initializes a ring buffer in the given memory chunk
    static RingBufferHeader* init(void* memory, uint32_t capacity, uint32_t item_size);
    
    // Returns the total size needed in bytes
    static size_t calculate_size(uint32_t capacity, uint32_t item_size);

    RingBuffer(RingBufferHeader* header);

    // Write a single item. Returns false if full.
    bool push(const void* item);
    
    // Read a single item. Returns false if empty.
    bool pop(void* item);
    
    // Check if empty
    bool is_empty() const;
    
    // Check if full
    bool is_full() const;
    
    // Get pointer to the raw buffer
    void* get_buffer() const;
    
    // Wait for the buffer to have items (Consumer wait)
    // Uses WebAssembly atomic wait internally via Emscripten
    void wait_until_not_empty();
    
    // Notify a waiting thread that the buffer has items
    void notify();

private:
    RingBufferHeader* header_;
    uint8_t* buffer_data_;
};

} // namespace NetzWirbel

namespace ntzwrbl = NetzWirbel;
