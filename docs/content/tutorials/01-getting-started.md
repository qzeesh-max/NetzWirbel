# Tutorial 1: Getting Started

This tutorial covers the basic setup of NetzWirbel, initializing the C++ Context, and connecting it to the JavaScript bridge.

## 1. Setting up the C++ Context

In your C++ WebAssembly application, you need to create an instance of `NetzWirbel::Context`. The Context requires two blocks of memory (one for C++ to JS, and one for JS to C++) and the capacity of the ring buffers.

```cpp
#include "NetzWirbel/Context.hpp"
#include <emscripten.h>

// Global context pointer
NetzWirbel::Context* g_ctx = nullptr;

extern "C" {
    // Exported function for JS to initialize the C++ Context.
    // JavaScript dynamically fetches sizes, allocates memory, 
    // and passes the memory pointers into this function.
    EMSCRIPTEN_KEEPALIVE
    void netzwirbel_init(void* cpp_to_js_mem, void* js_to_cpp_mem, uint32_t capacity) {
        if (g_ctx) {
            delete g_ctx;
        }
        // Initialize the Context using the memory allocated by JS
        g_ctx = new NetzWirbel::Context(cpp_to_js_mem, js_to_cpp_mem, capacity);
    }
}
```

## 2. Setting up the JavaScript Bridge

On the JavaScript side, you need to instantiate the `NetzWirbelBridge` and provide it with the WebAssembly memory and the offsets you allocated in C++.

```javascript
// Assuming your Wasm module is loaded into a variable `wasmModule`
// and you have included `netzwirbel_bridge.js`

window.setupBridge = function(capacity) {
    const memory = wasmModule.memory;
    const mallocFn = wasmModule._malloc;
    const freeFn = wasmModule._free;
    
    // Dynamically fetch exact structure sizes from Wasm
    const cmdSize = wasmModule._netzwirbel_get_command_size();
    const evSize = wasmModule._netzwirbel_get_event_msg_size();
    const headerSize = wasmModule._netzwirbel_get_ring_buffer_header_size();
    
    // Allocate memory on the WebAssembly heap
    const cppToJsOffset = mallocFn(capacity * cmdSize + headerSize);
    const jsToCppOffset = mallocFn(capacity * evSize + headerSize);
    
    const layout = {
        cmdSize: cmdSize,
        evSize: evSize,
        headerSize: headerSize,
        headOffset: wasmModule._netzwirbel_get_ring_buffer_head_offset(),
        tailOffset: wasmModule._netzwirbel_get_ring_buffer_tail_offset()
    };
    
    // Tell C++ to initialize the context with our allocated memory
    wasmModule._netzwirbel_init(cppToJsOffset, jsToCppOffset, capacity);
    
    // Create the bridge
    const bridge = new NetzWirbelBridge(memory, cppToJsOffset, jsToCppOffset, capacity, mallocFn, freeFn, layout);
    
    // Start the polling loop
    startNetzWirbelLoop(bridge);
};
```

> [!IMPORTANT]
> The `startNetzWirbelLoop` function uses `requestAnimationFrame` to continuously poll the C++ to JS ring buffer and process incoming commands efficiently.

Now that the bridge is set up, you can start creating elements and modifying the DOM from C++! Proceed to the next tutorial.
