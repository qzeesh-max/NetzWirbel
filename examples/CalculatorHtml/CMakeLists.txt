add_executable(CalculatorHtml main.cpp)
target_link_libraries(CalculatorHtml PRIVATE NetzWirbel)

set_target_properties(CalculatorHtml PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if (EMSCRIPTEN)
    set_target_properties(CalculatorHtml PROPERTIES
        LINK_FLAGS "-s EXPORTED_FUNCTIONS=_main,_malloc,_free,_netzwirbel_init,_netzwirbel_tick -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,wasmMemory -s ALLOW_MEMORY_GROWTH=1"
    )
endif()

# Copy index.html to the build directory
add_custom_command(TARGET CalculatorHtml POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html
        ${CMAKE_CURRENT_BINARY_DIR}/index.html
)
