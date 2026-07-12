# Tutorial 5: Banzai Exchange (FIX Protocol)

NetzWirbel supports building sophisticated web applications, including financial trading interfaces. The `BanzaiExchange` example demonstrates how to build a full-featured FIX (Financial Information eXchange) client running entirely within the browser via WebAssembly.

## 1. Connecting to the FIX Engine

The example utilizes Emscripten's WebSocket API to connect to an `OrderMatchBackend` instance running locally via a proxy.

```cpp
EmscriptenWebSocketCreateAttributes ws_attrs = {
    "ws://localhost:3000/fix",
    NULL, EM_TRUE
};
ws_ = emscripten_websocket_new(&ws_attrs);
emscripten_websocket_set_onopen_callback(ws_, this, onOpen);
emscripten_websocket_set_onmessage_callback(ws_, this, onMessage);
```

## 2. In-Browser FIX Parsing

Instead of relying on JavaScript to parse FIX messages, the example implements a fast, zero-allocation C++ `FixParser`. When a message is received over the WebSocket, it is parsed directly in WebAssembly:

```cpp
FixMessage msg = FixMessage::parse(raw_string);
if (msg.msgType == "W") { // Market Data Snapshot Full Refresh
    app->handleMarketData(msg);
} else if (msg.msgType == "8") { // Execution Report
    app->handleExecutionReport(msg);
}
```

## 3. Dynamic UI Updates

The UI consists of an Order Entry panel, a Market Data grid, and an Active Orders grid. `BanzaiExchange` heavily utilizes the new `Context::register_string()` API to reuse string handles across the C++ and JS boundary, avoiding redundant string allocations for HTML element names, CSS styles, and values:

```cpp
auto mdTrh = std::make_shared<Element>(ctx, ctx->strings.tr);
ctx->register_element(mdTrh);
```

This demonstrates how NetzWirbel can be used to construct robust trading frontends running complex business logic (such as FIX session management and message routing) directly in C++ while manipulating the DOM with native performance.
