# Tutorial 1: Getting Started

This tutorial covers the basic setup of NetzWirbel, initializing the C++ Context, and connecting it to the JavaScript bridge.

## 1. Setting up the C++ Context

In your C++ WebAssembly application, you need to create an instance of `NetzWirbel::Context`. The Context requires two blocks of memory (one for C++ to JS, and one for JS to C++) and the capacity of the ring buffers.

```cpp
#include "NetzWirbel/Context.hpp"
#include <emscripten.h>
#include <malloc.h>

// Global context pointer
NetzWirbel::Context* g_ctx = nullptr;

extern "C" {
    // Exported function for JS to initialize the bridge
    EMSCRIPTEN_KEEPALIVE
    void init_netzwirbel() {
        uint32_t capacity = 1024;
        // Allocate memory for the ring buffers
        size_t buffer_size = NetzWirbel::RingBuffer::calculate_size(capacity, 64);
        void* cpp_to_js_mem = memalign(8, buffer_size);
        void* js_to_cpp_mem = memalign(8, buffer_size);

        // Initialize the Context
        g_ctx = new NetzWirbel::Context(cpp_to_js_mem, js_to_cpp_mem, capacity);
        
        // Pass pointers back to JS (using EM_ASM or letting JS read globals/returns)
        EM_ASM({
            window.setupBridge($0, $1, $2);
        }, cpp_to_js_mem, js_to_cpp_mem, capacity);
    }
}
```

## 2. Setting up the JavaScript Bridge

On the JavaScript side, you need to instantiate the `NetzWirbelBridge` and provide it with the WebAssembly memory and the offsets you allocated in C++.

```javascript
// Assuming your Wasm module is loaded into a variable `wasmModule`
// and you have included `netzwirbel_bridge.js`

window.setupBridge = function(cppToJsOffset, jsToCppOffset, capacity) {
    const memory = wasmModule.memory;
    const mallocFn = wasmModule._malloc; // Needed for string allocation
    
    // Create the bridge
    const bridge = new NetzWirbelBridge(memory, cppToJsOffset, jsToCppOffset, capacity, mallocFn);
    
    // Start the polling loop
    startNetzWirbelLoop(bridge);
};
```

> [!IMPORTANT]
> The `startNetzWirbelLoop` function uses `requestAnimationFrame` to continuously poll the C++ to JS ring buffer and process incoming commands efficiently.

Now that the bridge is set up, you can start creating elements and modifying the DOM from C++! Proceed to the next tutorial.
