cmake_minimum_required(VERSION 3.16)
project(OrderMatchBackend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../../deps/quickfix quickfix)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/quickfix/src/C++ ${CMAKE_CURRENT_BINARY_DIR}/include/quickfix)

add_executable(OrderMatchBackend
    Application.cpp
    Market.cpp
    ordermatch.cpp
)

target_include_directories(OrderMatchBackend PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/include 
    ../../deps/quickfix/src/C++ 
    ../../deps/quickfix/include)
target_link_libraries(OrderMatchBackend PRIVATE quickfix)

# We need a quickfix config file in the build directory
# but there's no ordermatch.cfg yet, actually quickfix examples might not have one here.
# Let's check what config it uses.