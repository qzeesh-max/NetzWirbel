cmake_minimum_required(VERSION 3.16)
project(NetzWirbel CXX)

add_compile_options("-D__is_consteval_only(x)=false" "-freflection")

set(CMAKE_CXX_STANDARD 26)

# Core framework library
add_library(NetzWirbel STATIC
    src/RingBuffer.cpp
    src/Context.cpp
    src/DOM/Element.cpp
    src/DOM/Window.cpp
    src/DOM/Button.cpp
    src/DOM/Spinner.cpp
    src/DOM/TextBox.cpp
    src/DOM/TextArea.cpp
    src/DOM/RichTextArea.cpp
    src/Network.cpp
    src/App.cpp
)
target_include_directories(NetzWirbel PUBLIC include)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NETZWIRBEL_ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(NETZWIRBEL_ENABLE_COVERAGE)
    message(STATUS "Coverage enabled. Adding compiler flags...")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

if(EMSCRIPTEN)
    # Required for SharedArrayBuffer and atomics
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    # Export malloc/free for JS to allocate strings, and UTF8 utilities
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -s PTHREAD_POOL_SIZE=2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8 -s EXPORTED_FUNCTIONS=_malloc,_free,_main")
endif()

add_subdirectory(examples)
add_subdirectory(tests)
