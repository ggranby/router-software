@echo off
setlocal EnableExtensions

set "GEN=%~1"
if "%GEN%"=="" set "GEN=ZIP"

set "CONFIG=Release"
if not "%~2"=="" set "CONFIG=%~2"

set "CMAKE_BIN="
where cmake >NUL 2>&1
if not errorlevel 1 set "CMAKE_BIN=cmake"
if "%CMAKE_BIN%"=="" if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files\CMake\bin\cmake.exe"
if "%CMAKE_BIN%"=="" if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" set "CMAKE_BIN=C:\Program Files (x86)\CMake\bin\cmake.exe"

if "%CMAKE_BIN%"=="" (
    echo [ERROR] Could not find cmake executable.
    exit /b 1
)

set "CPACK_BIN="
where cpack >NUL 2>&1
if not errorlevel 1 set "CPACK_BIN=cpack"
if "%CPACK_BIN%"=="" if exist "C:\Program Files\CMake\bin\cpack.exe" set "CPACK_BIN=C:\Program Files\CMake\bin\cpack.exe"
if "%CPACK_BIN%"=="" if exist "C:\Program Files (x86)\CMake\bin\cpack.exe" set "CPACK_BIN=C:\Program Files (x86)\CMake\bin\cpack.exe"

if "%CPACK_BIN%"=="" (
    echo [ERROR] Could not find cpack executable.
    exit /b 1
)

if /I "%GEN%"=="NSIS" (
    where makensis >NUL 2>&1
    if errorlevel 1 (
        echo [ERROR] NSIS generator requested, but makensis was not found.
        echo [HINT] Install NSIS or run: make-installer.cmd ZIP
        exit /b 1
    )
)

echo [INFO] Configuring project...
"%CMAKE_BIN%" -S . -B build
if errorlevel 1 exit /b 1

echo [INFO] Building hornet-link (%CONFIG%)...
"%CMAKE_BIN%" --build build --config %CONFIG%
if errorlevel 1 exit /b 1

echo [INFO] Packaging with CPack generator: %GEN%
pushd build >NUL
"%CPACK_BIN%" -C %CONFIG% -G %GEN%
set "CPACK_ERR=%ERRORLEVEL%"
popd >NUL
if not "%CPACK_ERR%"=="0" exit /b %CPACK_ERR%

echo [OK] Package generated under build\ (CPack output).
exit /b 0
