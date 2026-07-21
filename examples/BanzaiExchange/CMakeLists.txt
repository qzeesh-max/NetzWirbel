add_executable(BanzaiExchange main.cpp FixParser.cpp)
target_link_libraries(BanzaiExchange PRIVATE NetzWirbel)
target_include_directories(BanzaiExchange PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/quickfix/src/C++
)

set_target_properties(BanzaiExchange PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (EMSCRIPTEN)
    set_target_properties(BanzaiExchange PROPERTIES
        LINK_FLAGS "-lwebsocket.js -s EXPORTED_FUNCTIONS=_main,_malloc,_free,_netzwirbel_init,_netzwirbel_tick,_netzwirbel_get_ring_buffer_header_size,_netzwirbel_get_command_size,_netzwirbel_get_event_msg_size,_netzwirbel_get_ring_buffer_head_offset,_netzwirbel_get_ring_buffer_tail_offset -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,wasmMemory -s ALLOW_MEMORY_GROWTH=1"
    )
endif()

# Copy index.html to the build directory
add_custom_command(TARGET BanzaiExchange POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
)
