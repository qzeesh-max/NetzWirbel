# NetzWirbel Overview

Welcome to the **NetzWirbel** documentation! 

NetzWirbel is a lightweight, high-performance C++ to JavaScript DOM bridge. It is designed to work with WebAssembly and uses lock-free SharedArrayBuffer ring buffers to facilitate ultra-fast, zero-copy (where possible) communication between a C++ WebAssembly module and the host browser's JavaScript environment.

## Why NetzWirbel?

When running C++ code in the browser using WebAssembly (via Emscripten), interacting with the DOM usually requires expensive JavaScript interop calls. NetzWirbel solves this by batching commands and events into shared memory ring buffers, minimizing the overhead of crossing the Wasm-JS boundary.

## Architecture

At a high level, the architecture consists of two main components:
1. **The C++ Context**: Manages the C++ representations of DOM elements and pushes DOM mutation commands (like `CREATE_ELEMENT`, `SET_ATTRIBUTE`) into a ring buffer.
2. **The JavaScript Bridge (`NetzWirbelBridge`)**: Runs in the browser, polls the ring buffer for commands, applies them to the actual browser DOM, and pushes user events (like `click` or `input`) back into another ring buffer for C++ to consume.

> [!TIP]
> Because it relies on `SharedArrayBuffer` and `Atomics`, your web server must send the appropriate cross-origin isolation headers (`Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp`).

## Prerequisites

To build and run NetzWirbel and its examples, you need the following installed:
- **Emscripten SDK (emsdk)**: Used to compile C++ to WebAssembly.
- **CMake**: Build system generation.
- **Node.js & npm**: Required to run tests, coverage reporting, and the local documentation server.

For running **Unit Tests and Coverage**:
- **lcov**: Required to generate the HTML coverage reports. On macOS, this can be installed via `brew install lcov`.
- **genhtml**: Included as part of the `lcov` package.

## Testing & Coverage

NetzWirbel uses **Google Test** for unit testing C++ code, executed via Emscripten and Node.js.
The test suite is organized into modular categories (`core`, `dom`, `network`).

To run tests with coverage reporting:
```bash
# This script ensures lcov is installed, builds the tests, runs them,
# generates coverage using lcov/genhtml, and starts the server on port 8080.
./run_coverage.sh
```

You can view the coverage report locally at `http://localhost:8080`.

Explore the **Tutorials** to learn how to set up and use NetzWirbel, or dive into the **API Reference** for detailed class documentation.
