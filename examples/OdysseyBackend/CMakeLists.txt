cmake_minimum_required(VERSION 3.16)
project(OdysseyBackend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(OdysseyBackend
    main.cpp
    Market.cpp
)

target_include_directories(OdysseyBackend PRIVATE .)

if(WIN32)
    target_link_libraries(OdysseyBackend ws2_32)
endif()
