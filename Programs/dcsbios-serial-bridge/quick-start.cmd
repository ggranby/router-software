@echo off
setlocal EnableExtensions

rem One-command bootstrap + launch for Hornet Link bridge.
rem Usage examples:
rem   quick-start.cmd
rem   quick-start.cmd --dry-run --autostart
rem   quick-start.cmd --ports=3,4,5 --autostart
rem   quick-start.cmd --rebuild --ports=10

set "CONFIG=Release"
set "REBUILD=0"
set "PASS_ARGS="

:parse
if "%~1"=="" goto after_parse
if /I "%~1"=="--rebuild" (
    set "REBUILD=1"
) else (
    set "PASS_ARGS=%PASS_ARGS% %~1"
)
shift
goto parse

:after_parse
set "CMAKE_BIN="
where cmake >NUL 2>&1
if not errorlevel 1 set "CMAKE_BIN=cmake"
if "%CMAKE_BIN%"=="" if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files\CMake\bin\cmake.exe"
if "%CMAKE_BIN%"=="" if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files (x86)\CMake\bin\cmake.exe"
if "%CMAKE_BIN%"=="" if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

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

if not exist build\CMakeCache.txt (
    echo [INFO] Configuring project with generator: %CMAKE_GENERATOR%
    "%CMAKE_BIN%" -S . -B build -G "%CMAKE_GENERATOR%"
    if errorlevel 1 exit /b 1
)

echo [INFO] Building hornet-link (%CONFIG%)...
"%CMAKE_BIN%" --build build --config %CONFIG%
if errorlevel 1 exit /b 1

set "EXE=build\%CONFIG%\hornet-link.exe"
if not exist "%EXE%" (
    echo [ERROR] Build succeeded but executable was not found at %EXE%
    exit /b 1
)

echo [OK] Launching %EXE% %PASS_ARGS%
start "hornet-link" "%EXE%" %PASS_ARGS%

exit /b 0
