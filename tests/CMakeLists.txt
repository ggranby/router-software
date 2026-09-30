cmake_minimum_required(VERSION 3.20)
project(hornet-link-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HORNET_LINK_SANITIZE "Build tests with AddressSanitizer + UBSan (GCC/Clang)" OFF)
option(HORNET_LINK_FUZZ "Build libFuzzer targets (Clang only)" OFF)

set(HORNET_LINK_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Programs/dcsbios-serial-bridge/src)
# Portable parts of the Arduino library (protocol v2 core, catalogue, filters).
set(HORNET_LINK_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/HornetLink/src)

add_executable(hornet-link-tests
    test_main.cpp
    test_core.cpp
    test_profile_store.cpp
    test_hornet_v2.cpp
)

target_include_directories(hornet-link-tests PRIVATE ${HORNET_LINK_SRC_DIR} ${HORNET_LINK_LIB_DIR})

if(WIN32)
    target_compile_definitions(hornet-link-tests PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

if(MSVC)
    target_compile_options(hornet-link-tests PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(hornet-link-tests PRIVATE -Wall -Wextra -Werror)
endif()

if(HORNET_LINK_SANITIZE AND NOT MSVC)
    target_compile_options(hornet-link-tests PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(hornet-link-tests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
add_test(NAME hornet-link-tests COMMAND hornet-link-tests)

if(HORNET_LINK_FUZZ)
    add_subdirectory(fuzz)
endif()
