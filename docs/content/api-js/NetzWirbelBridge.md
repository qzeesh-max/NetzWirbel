# NetzWirbelBridge (JavaScript)

The `NetzWirbelBridge` class is the main Javascript orchestrator. It sits in the browser, reads commands from C++, manipulates the browser DOM, and sends events back.

## Constructor

```javascript
new NetzWirbelBridge(wasmMemory, cppToJsOffset, jsToCppOffset, capacity, mallocFn, freeFn, layout)
```
- **`wasmMemory`**: The `WebAssembly.Memory` instance exported by your Wasm module.
- **`cppToJsOffset`**: The byte offset where the Command ring buffer starts.
- **`jsToCppOffset`**: The byte offset where the EventMsg ring buffer starts.
- **`capacity`**: Maximum capacity of the ring buffers (must match C++ capacity).
- **`mallocFn`**: Reference to the Wasm module's `_malloc` function, used to allocate memory for strings passed from JS back to C++.
- **`freeFn`**: Reference to the Wasm module's `_free` function, used to avoid memory leaks.
- **`layout`**: Optional object to dynamically map the C++ `Command` sizes, `EventMsg` sizes, and `RingBufferHeader` sizes instead of using hardcoded magic numbers.

## Main Loop

### `startNetzWirbelLoop(bridge)`
A global utility function that starts a `requestAnimationFrame` loop. 
```javascript
function startNetzWirbelLoop(bridge) {
    function loop() {
        bridge.processCommands();
        requestAnimationFrame(loop);
    }
    loop();
}
```

## Internal Methods

- **`processCommands()`**: Pops all available commands from the `RingBufferConsumer` and delegates to `handleCommand`.
- **`handleCommand(type, targetId, ...)`**: Contains the core `switch` statement that executes DOM mutations based on the `CommandType`.
- **`sendEvent(targetId, e)`**: Serializes a standard DOM event and pushes an `EventMsg` to the `jsToCpp` producer.
- **`sendPropertyChangeString(targetId, propName, value)`**: Used to implement two-way bindings (like `<input>` values), sending updates immediately to C++.

## Lock-Free Conflation

For extreme throughput updates (e.g., market data ticks), `NetzWirbelBridge` supports three lock-free amortized O(1) conflation commands:
- `SET_TEXT_CONTENT_CONFLATED`
- `SET_CLASS_CONFLATED`
- `SET_STYLE_CONFLATED`

These commands utilize the shared `WebAssembly.Memory` buffer (`Atomics.exchange`) to extract lengths and pointers for strings that have been written atomically by C++. This avoids queuing redundant DOM mutations when the browser tab is heavily loaded or asleep.
