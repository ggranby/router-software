cmake_minimum_required(VERSION 3.20)
project(hornet_link LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hornet-link WIN32
    src/main.cpp
    src/BiosProtocol.hpp
    src/ControlDatabase.hpp
    src/DeviceRegistry.hpp
    src/SimSource.hpp
    src/DcsBiosSource.hpp
    src/DcsDirectSource.hpp
    src/ReplayFileSource.hpp
    src/MsfsSource.hpp
    src/ProfileStore.hpp
    src/RS485ProtocolSpec.hpp
)

target_compile_definitions(hornet-link PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(hornet-link PRIVATE
    ws2_32
)

if(MSVC)
    target_compile_options(hornet-link PRIVATE /W4 /permissive-)
endif()
