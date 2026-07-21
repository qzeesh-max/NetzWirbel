# JS RingBuffers

To complement the C++ `RingBuffer`, the JavaScript side provides a `RingBufferConsumer` and a `RingBufferProducer` which use `Atomics` to safely read and write to the `SharedArrayBuffer` memory.

## `RingBufferConsumer`

Used to read commands coming from C++.

- **`constructor(wasmMemory, headerOffset, capacity, itemSize, layout = {})`**
  - **`layout`**: Optional object to dynamically configure sizes and offsets. Properties: `headerSize`, `headOffset`, `tailOffset`. Defaults to a 384-byte padded header.
- **`isEmpty()`**: Uses `Atomics.load` to check if `head === tail`.
- **`pop()`**: Returns a `DataView` of the copied item memory, and increments the `tail`.

## `RingBufferProducer`

Used to write events going to C++.

- **`constructor(wasmMemory, headerOffset, capacity, itemSize, layout = {})`**
  - **`layout`**: Optional object to dynamically configure sizes and offsets.
- **`isFull()`**: Uses `Atomics.load` to check if the next `head` equals the `tail`.
- **`push(dataView)`**: Copies the `dataView` bytes into the buffer, increments the `head`, and calls `Atomics.notify(this.headArray, 0, 1)` to wake up any C++ threads waiting on the buffer.
