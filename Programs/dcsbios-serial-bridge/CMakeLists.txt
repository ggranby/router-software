cmake_minimum_required(VERSION 3.20)
project(hornet_link LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HORNET_LINK_ENABLE_IMGUI "Build optional Dear ImGui UI prototype target" OFF)

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

set_target_properties(hornet-link PROPERTIES
    OUTPUT_NAME "hornet-link"
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

# Copy seed configuration files to build directory
add_custom_command(TARGET hornet-link POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device_profiles.json
        $<TARGET_FILE_DIR:hornet-link>/device_profiles.json
    COMMENT "Copying device_profiles.json to build output directory"
)

if(HORNET_LINK_ENABLE_IMGUI)
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
    if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
        message(FATAL_ERROR
            "HORNET_LINK_ENABLE_IMGUI=ON but Dear ImGui was not found at ${IMGUI_DIR}. "
            "Run setup-imgui.cmd in this folder first.")
    endif()

    add_executable(hornet-link-imgui WIN32
        src/main_imgui.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )

    set_target_properties(hornet-link-imgui PROPERTIES
        OUTPUT_NAME "hornet-link-imgui"
    )

    target_include_directories(hornet-link-imgui PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )

    target_compile_definitions(hornet-link-imgui PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    target_link_libraries(hornet-link-imgui PRIVATE
        d3d11
        dxgi
    )

    if(MSVC)
        target_compile_options(hornet-link-imgui PRIVATE /W4 /permissive-)
    endif()
endif()

# Install + CPack packaging (ZIP by default; NSIS optional when available)
install(TARGETS hornet-link
    RUNTIME DESTINATION .
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/device_profiles.json
    ${CMAKE_CURRENT_SOURCE_DIR}/quick-start.cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION .
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE")
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE
        DESTINATION .
    )
endif()

set(CPACK_PACKAGE_NAME "hornet-link")
set(CPACK_PACKAGE_VENDOR "Hornet Link")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Hornet Link")
set(CPACK_PACKAGE_FILE_NAME "hornet-link-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_MONOLITHIC_INSTALL ON)
set(CPACK_GENERATOR "ZIP")

include(CPack)
