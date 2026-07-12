# NetzWirbel

NetzWirbel is a C++ web application framework using WebAssembly via Emscripten.

## Prerequisites

To build and run this project, you need the following installed:
- **Emscripten SDK (emsdk)**: Required to compile the C++ frontend into WebAssembly.
- **CMake** (v3.16+): The build configuration system.
- **Make / Ninja / MSVC**: Your platform-specific build tool.
- **Node.js & npm**: Required to run the local servers, execute tests, and serve examples.
- **C++ Compiler (GCC, Clang, or MSVC)**: Required for building the native backend examples.
- **Git**: For source control and cloning submodules.
- **lcov & genhtml**: (Optional) For generating test coverage reports (available via Homebrew/Linux package managers).

## External Dependencies / Submodules

This project depends on the [Quickfix](https://github.com/quickfix/quickfix) library via a git submodule located at `deps/quickfix`.
- **Tested Commit:** `386ce46e917ae494ab6e90b1be90fd421cdbe3f9` (v1.15.1-354-g386ce46e).

Make sure to initialize and update submodules after cloning the repository:
```bash
git submodule update --init --recursive
```

## Build Instructions

### macOS & Linux

We provide a script to build everything (the WebAssembly frontend and native backends):
```bash
./build_all.sh
```

Alternatively, to build just the NetzWirbel Emscripten components manually:
```bash
emcmake cmake -S . -B build
make -C build -j4
```

### Windows

Use the provided command scripts to build all components:
```cmd
.\build_all.cmd
```

Alternatively, to build just the NetzWirbel Emscripten components manually:
```cmd
emcmake cmake -S . -B build
cmake --build build -j 4
```

## Examples

The examples consist of WebAssembly frontends and native C++ backends (like `OrderMatchBackend` and `OdysseyBackend`).

### Build Instructions
The examples are automatically built when you run `build_all.sh` (macOS/Linux) or `build_all.cmd` (Windows) in the base directory.

### Run Instructions

#### 1. Frontend Orchestrator
To serve the WebAssembly frontend examples, start the orchestrator server:
```bash
npm install
npm run examples
```
*You can access the examples in your browser usually at `http://localhost:3000`.*

#### 2. Native Backends
To provide backend data and connections to the web examples, run the native backends in separate terminals alongside the orchestrator.

**macOS / Linux:**
```bash
./examples/OrderMatchBackend/build-native/OrderMatchBackend examples/OrderMatchBackend/ordermatch.cfg
./examples/OdysseyBackend/build-native/OdysseyBackend
```

**Windows:**
```cmd
.\build_ordermatch\OrderMatchBackend.exe examples\OrderMatchBackend\ordermatch.cfg
.\build_odyssey\OdysseyBackend.exe
```

## Unit Tests

The project uses GoogleTest, which is automatically fetched by CMake. Because the framework targets WebAssembly, the compiled test binaries are executed using Node.js.

### macOS & Linux
To build and run tests:
```bash
emcmake cmake -S . -B build
make -C build -j4
cd build/tests
ctest --output-on-failure

# Or run individually via Node:
node RingBufferTest.js
```

### Windows
```cmd
emcmake cmake -S . -B build
cmake --build build -j 4
cd build\tests
ctest -C Debug --output-on-failure

# Or run individually via Node:
node RingBufferTest.js
```

## Servers

### Documentation Server
To launch the local documentation server:
```bash
# Using Node
node docs_server.js

# Or using the convenience scripts
./run_docs.sh  # macOS/Linux
.\run_docs.cmd # Windows
```

### Coverage Server
To generate and view test coverage reports in a local web server:
```bash
# macOS/Linux convenience script
./run_coverage.sh

# Or via NPM manually:
# 1. Generate the coverage report
npm run test:coverage

# 2. Start the coverage server to view the report
npm run coverage
```
*Note: Generating coverage requires `lcov` and `genhtml` to be installed on your system.*
