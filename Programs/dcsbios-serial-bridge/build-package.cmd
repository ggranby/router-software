@echo off
setlocal EnableExtensions

set CONFIG=Release
if not "%1"=="" set CONFIG=%1

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

if exist build (
    rmdir /s /q build
)

echo [INFO] Using CMake generator: %CMAKE_GENERATOR%
"%CMAKE_BIN%" -S . -B build -G "%CMAKE_GENERATOR%"
if errorlevel 1 exit /b 1

"%CMAKE_BIN%" --build build --config %CONFIG%
if errorlevel 1 exit /b 1

set "EXE_SOURCE=build\%CONFIG%\hornet-link.exe"
if not exist "%EXE_SOURCE%" (
    echo [ERROR] Built executable not found: %EXE_SOURCE%
    exit /b 1
)

set "PACKAGE_ROOT=build\package"
set "PACKAGE_DIR=%PACKAGE_ROOT%\hornet-link-%CONFIG%"
set "DIST_DIR=dist"
set "ZIP_PATH=%DIST_DIR%\hornet-link-%CONFIG%.zip"
set "JSON_SOURCE="

if exist "%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json\*.json" set "JSON_SOURCE=%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json"
if "%JSON_SOURCE%"=="" if exist "%USERPROFILE%\Saved Games\DCS.openbeta\Scripts\DCS-BIOS\doc\json\*.json" set "JSON_SOURCE=%USERPROFILE%\Saved Games\DCS.openbeta\Scripts\DCS-BIOS\doc\json"
if "%JSON_SOURCE%"=="" if exist "%USERPROFILE%\Saved Games\DCS.openalpha\Scripts\DCS-BIOS\doc\json\*.json" set "JSON_SOURCE=%USERPROFILE%\Saved Games\DCS.openalpha\Scripts\DCS-BIOS\doc\json"
if "%JSON_SOURCE%"=="" if exist "..\..\Scripts\DCS-BIOS\doc\json\*.json" set "JSON_SOURCE=..\..\Scripts\DCS-BIOS\doc\json"

if exist "%PACKAGE_ROOT%" (
    rmdir /s /q "%PACKAGE_ROOT%"
)

mkdir "%PACKAGE_DIR%"
if errorlevel 1 exit /b 1

copy /y "%EXE_SOURCE%" "%PACKAGE_DIR%\hornet-link.exe" >NUL
if errorlevel 1 exit /b 1

copy /y "build\%CONFIG%\device_profiles.json" "%PACKAGE_DIR%\device_profiles.json" >NUL
if errorlevel 1 exit /b 1

copy /y "quick-start.cmd" "%PACKAGE_DIR%\quick-start.cmd" >NUL
if errorlevel 1 exit /b 1

copy /y "README.md" "%PACKAGE_DIR%\README.md" >NUL
if errorlevel 1 exit /b 1

if exist "..\..\LICENSE" (
    copy /y "..\..\LICENSE" "%PACKAGE_DIR%\LICENSE" >NUL
)

if not "%JSON_SOURCE%"=="" (
    xcopy "%JSON_SOURCE%" "%PACKAGE_DIR%\json\" /E /I /Y >NUL
    if errorlevel 1 exit /b 1
) else (
    echo [WARN] JSON control database not found in Saved Games or repository doc folder.
)

if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

if exist "%ZIP_PATH%" del /q "%ZIP_PATH%"

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force"
if errorlevel 1 (
    echo [ERROR] Failed to create zip archive: %ZIP_PATH%
    exit /b 1
)

echo [OK] Ready to run: %PACKAGE_DIR%\hornet-link.exe
echo [OK] Release zip: %ZIP_PATH%
