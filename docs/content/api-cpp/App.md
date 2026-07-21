# NetzWirbel::App

The `App` class is the foundational base class for building applications using NetzWirbel. It abstracts away the low-level Emscripten lifecycle management (`netzwirbel_init` and `netzwirbel_tick`) and provides a structured, object-oriented way to initialize your application and receive tick updates.

## Architecture

By subclassing `App`, you avoid dealing with `EMSCRIPTEN_KEEPALIVE` and C-style exports. Instead, you simply implement the virtual methods, and use `NetzWirbel::run_app()` in your `main()` function to bootstrap the application.

## Virtual Methods

### `virtual void on_init(Context* ctx) = 0;`
Called once when the WebAssembly module is loaded and the NetzWirbel bridge is ready.
You must implement this method to construct your UI tree using the provided `Context`.

- **`ctx`**: A pointer to the `Context` object managing the DOM mapping. You should register your elements with this context.

### `virtual void on_tick(double time)`
Called repeatedly by the browser's requestAnimationFrame loop (or a polling loop).
The default implementation calls `ctx->process_events()` and `ctx->send_ping()`. If you override this, ensure you still pump events!

- **`time`**: The current DOMHighResTimeStamp passed by `requestAnimationFrame`.

## Bootstrapping (`run_app`)

Once you've defined your `App` subclass, you bootstrap it in your `main.cpp`:

```cpp
#include "MyApp.hpp"

int main() {
    NetzWirbel::run_app(std::make_unique<MyApp>());
    return 0;
}
```

Behind the scenes, `run_app` will:
1. Block the main thread from exiting using `emscripten_exit_with_live_runtime()`.
2. Manage the `Context` allocation when `netzwirbel_init` is called from JS.
3. Automatically route `netzwirbel_init` to `on_init()`.
4. Automatically route `netzwirbel_tick` to `on_tick()`.

In addition to `netzwirbel_init` and `netzwirbel_tick`, NetzWirbel exports several `_netzwirbel_get_*` size and offset functions so the JavaScript frontend can dynamically allocate the exact memory required for the `RingBuffer` without relying on magic numbers.
