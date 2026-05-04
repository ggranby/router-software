@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem One-command bootstrap + launch for Hornet Link bridge.
rem Usage examples:
rem   quick-start.cmd
rem   quick-start.cmd --dry-run --autostart
rem   quick-start.cmd --ports=3,4,5 --autostart
rem   quick-start.cmd --rebuild --ports=10
rem   quick-start.cmd --imgui

set "CONFIG=Release"
set "REBUILD=0"
set "USE_IMGUI=0"
set "PASS_ARGS="

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="--rebuild" (
    set "REBUILD=1"
    shift
    goto parse
)
if /I "%~1"=="--imgui" (
    set "USE_IMGUI=1"
    shift
    goto parse
)
set "PASS_ARGS=!PASS_ARGS! %~1"
shift
goto parse

:parsed
set "CMAKE_BIN="
where cmake >NUL 2>&1
if not errorlevel 1 set "CMAKE_BIN=cmake"
if "%CMAKE_BIN%"=="" (
    if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files\CMake\bin\cmake.exe"
)
if "%CMAKE_BIN%"=="" (
    if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files (x86)\CMake\bin\cmake.exe"
)
if "%CMAKE_BIN%"=="" (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)

if "%CMAKE_BIN%"=="" (
    echo [ERROR] Could not find cmake executable.
    echo Install CMake or add it to PATH.
    exit /b 1
)

set "CMAKE_GENERATOR=Visual Studio 17 2022"
set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%V in (`"%VSWHERE%" -latest -products * -property catalog_productLineVersion`) do set "VS_MAJOR=%%V"
)
if "%VS_MAJOR%"=="18" set "CMAKE_GENERATOR=Visual Studio 18 2026"
if "%VS_MAJOR%"=="17" set "CMAKE_GENERATOR=Visual Studio 17 2022"

if "%REBUILD%"=="1" (
    echo [INFO] Rebuild requested. Removing build directory...
    if exist build rmdir /s /q build
)

set "EXTRA_CMAKE="
if "%USE_IMGUI%"=="1" set "EXTRA_CMAKE=-DHORNET_LINK_ENABLE_IMGUI=ON"

echo [INFO] Configuring project with generator: %CMAKE_GENERATOR%
"%CMAKE_BIN%" -S . -B build -G "%CMAKE_GENERATOR%" %EXTRA_CMAKE%
if errorlevel 1 exit /b 1

if "%USE_IMGUI%"=="1" (
    echo [INFO] Building hornet-link-imgui (%CONFIG%)...
    "%CMAKE_BIN%" --build build --config %CONFIG% --target hornet-link-imgui
    if errorlevel 1 (
        echo [HINT] If ImGui target fails, run setup-imgui.cmd first.
        exit /b 1
    )
    set "EXE=build\%CONFIG%\hornet-link-imgui.exe"
) else (
    echo [INFO] Building hornet-link (%CONFIG%)...
    "%CMAKE_BIN%" --build build --config %CONFIG%
    if errorlevel 1 exit /b 1
    set "EXE=build\%CONFIG%\hornet-link.exe"
)

if not exist "%EXE%" (
    echo [ERROR] Build succeeded but executable was not found at %EXE%
    exit /b 1
)

echo [OK] Launching %EXE%%PASS_ARGS%
start "hornet-link" "%EXE%" %PASS_ARGS%

exit /b 0
