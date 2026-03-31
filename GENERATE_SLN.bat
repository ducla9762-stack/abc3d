@echo off
setlocal

REM ----------------------------
REM Configuration
REM ----------------------------
set BUILD_DIR=build
set GENERATOR="Visual Studio 17 2022"
set ARCH=x64

REM Path to vcpkg toolchain (adjust if needed)
set VCPKG_TOOLCHAIN=%CD%\vcpkg\scripts\buildsystems\vcpkg.cmake

REM ----------------------------
REM Configure step
REM ----------------------------
echo Configuring CMake project...

cmake -S . -B %BUILD_DIR% ^
    -G %GENERATOR% ^
    -A %ARCH% ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN% ^
    -DVCPKG_TARGET_TRIPLET=x64-windows

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b %errorlevel%
)

REM ----------------------------
REM Build step (optional but useful)
REM ----------------------------
echo Building project...

cmake --build %BUILD_DIR% --config Debug

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo.
echo Done successfully!
pause