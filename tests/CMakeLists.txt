include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Helper function to create a test executable
function(netzwirbel_add_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE NetzWirbel GTest::gtest_main)
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../deps/quickfix/src/C++
    )
    target_link_options(${test_name} PRIVATE 
        "-sNODERAWFS=1" 
        "-g"
        "-sASSERTIONS=2"
        "-sEXIT_RUNTIME=1"
        "-lwebsocket.js"
    )
    set_target_properties(${test_name} PROPERTIES SUFFIX ".js")
    
    # Register the test to be run with ctest (though we'll use node to run them directly or via ctest)
    add_test(NAME ${test_name} COMMAND node ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.js)
endfunction()

netzwirbel_add_test(RingBufferTest core/RingBufferTest.cpp)
netzwirbel_add_test(ExamplesTest examples/test_examples.cpp)
netzwirbel_add_test(FixParserTest examples/FixParserTest.cpp)
netzwirbel_add_test(ContextTest core/ContextTest.cpp)
netzwirbel_add_test(ElementTest dom/ElementTest.cpp)
netzwirbel_add_test(NetworkTest network/NetworkTest.cpp)

if(NETZWIRBEL_ENABLE_COVERAGE)
    # Add a custom target to generate coverage report
    add_custom_target(coverage_report
        COMMAND rm -rf coverage_html
        COMMAND find . -name "*.gcda" -delete
        # Run tests explicitly via node or ctest
        COMMAND node RingBufferTest.js || true
        COMMAND node ContextTest.js || true
        COMMAND node ElementTest.js || true
        COMMAND node NetworkTest.js || true
        COMMAND node ExamplesTest.js || true
        COMMAND lcov --ignore-errors path,inconsistent,unsupported,empty,format,unused,category --directory . --capture --output-file coverage.info
        # Filter out googletest and system headers
        COMMAND lcov --ignore-errors path,inconsistent,unsupported,empty,format,unused,category --remove coverage.info '/usr/*' '*/_deps/googletest*' '*/tests/*' -o coverage_filtered.info
        COMMAND genhtml --ignore-errors path,inconsistent,unsupported,empty,format,unused,category coverage_filtered.info --output-directory coverage_html
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HTML coverage report..."
    )
endif()
