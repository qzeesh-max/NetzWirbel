# NetzWirbel::RingBuffer

A lock-free ring buffer implementation designed to work natively with WebAssembly's `SharedArrayBuffer` memory paradigm.

## `RingBufferHeader`
A structural header aligned to 8 bytes, placed at the start of the memory chunk.
```cpp
struct alignas(8) RingBufferHeader {
    std::atomic<uint32_t> head; // Index where producer writes
    std::atomic<uint32_t> tail; // Index where consumer reads
    uint32_t capacity;          // Must be a power of 2
    uint32_t item_size;         // Size of each item in bytes
    std::atomic<uint32_t> state; 
};
```

## `RingBuffer` Class

### `static size_t calculate_size(uint32_t capacity, uint32_t item_size)`
Helper to calculate the total byte size needed to allocate memory for the buffer.

### `bool push(const void* item)`
Writes an item into the buffer. Returns `false` if the buffer is full.

### `bool pop(void* item)`
Reads an item from the buffer. Returns `false` if the buffer is empty.

### `void wait_until_not_empty()` / `void notify()`
Uses internal WebAssembly `__builtin_wasm_memory_atomic_wait32` (via Emscripten/Atomics) for synchronization if polling is insufficient.
