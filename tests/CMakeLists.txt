cmake_minimum_required(VERSION 3.20)
project(hornet-link-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Simple test executable
add_executable(hornet-link-tests
    test_main.cpp
)

target_include_directories(hornet-link-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/../Programs/dcsbios-serial-bridge/src
)

target_link_libraries(hornet-link-tests PRIVATE
    ws2_32
)

target_compile_definitions(hornet-link-tests PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(hornet-link-tests PRIVATE /W4 /permissive-)
endif()

enable_testing()
add_test(NAME hornet-link-tests COMMAND hornet-link-tests)

