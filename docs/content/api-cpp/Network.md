# NetzWirbel::Network

The `Network` module provides an object-oriented `WebSocket` class for connecting to a WebSocket server directly from your C++ WebAssembly module. Under the hood, it uses Emscripten's `EM_JS` macro to instantiate native browser `WebSocket` connections and proxy messages back to C++.

> [!NOTE]
> Network messages bypass the main NetzWirbel `Command` and `EventMsg` ring buffers. Instead, they use direct `EM_JS` invocations to allocate memory on the Wasm heap and trigger a C callback, which is generally efficient enough for string-based data streams.

## Class: `WebSocket`

The `WebSocket` class encapsulates a single, uniquely-identified connection. You can maintain as many concurrent connections as you need. It requires an `NetzWirbel::Context` to properly coordinate its lifecycle, although messages are delivered asynchronously.

### Constructor

#### `WebSocket(Context* ctx, const std::string& url)`
Creates and immediately connects a new WebSocket to the specified endpoint.
- **Example**: `auto ws = std::make_shared<NetzWirbel::WebSocket>(ctx, "ws://localhost:3000");`

### Methods

#### `void send(const std::string& data)`
Sends a string payload over the active WebSocket connection.

#### `void close()`
Closes the WebSocket connection. This is also called automatically when the `WebSocket` object is destroyed.

#### `void on_message(std::function<void(const std::string&)> callback)`
Registers a C++ callback that is triggered whenever the WebSocket receives a message. 
The callback receives the entire text payload as a `std::string`.

```cpp
ws->on_message([](const std::string& msg) {
    std::cout << "Received message: " << msg << std::endl;
});
```

#### `void on_close(std::function<void()> callback)`
Registers a C++ callback that is triggered when the connection drops or is closed from the server side.

```cpp
ws->on_close([]() {
    std::cout << "Connection closed!" << std::endl;
});
```
