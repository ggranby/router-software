@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "IMGUI_DIR=%ROOT%third_party\imgui"

where git >NUL 2>&1
if errorlevel 1 (
    echo [ERROR] git is required to fetch Dear ImGui.
    exit /b 1
)

if exist "%IMGUI_DIR%\imgui.h" (
    echo [OK] Dear ImGui already present at "%IMGUI_DIR%".
    exit /b 0
)

if not exist "%ROOT%third_party" mkdir "%ROOT%third_party"

echo [INFO] Cloning Dear ImGui into third_party\imgui ...
git clone --depth 1 https://github.com/ocornut/imgui.git "%IMGUI_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to clone Dear ImGui.
    exit /b 1
)

echo [OK] Dear ImGui fetched successfully.
echo [INFO] Build prototype target with:
echo        cmake -S . -B build -DHORNET_LINK_ENABLE_IMGUI=ON
echo        cmake --build build --config Release --target hornet-link-imgui

exit /b 0
