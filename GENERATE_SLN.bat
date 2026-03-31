@echo off
setlocal

REM ----------------------------
REM Paths
REM ----------------------------
set VCPKG_DIR=%CD%\vcpkg
set TOOLCHAIN=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake
set BUILD_DIR=build

REM ----------------------------
REM Step 1: Ensure vcpkg exists
REM ----------------------------
if not exist "%VCPKG_DIR%" (
    echo vcpkg not found, cloning...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
    if %errorlevel% neq 0 (
        echo Failed to clone vcpkg!
        pause
        exit /b %errorlevel%
    )
) else (
    echo vcpkg folder exists
)

REM ----------------------------
REM Step 2: Init submodule (if used)
REM ----------------------------
echo Checking submodules...
git submodule update --init --recursive

REM ----------------------------
REM Step 3: Bootstrap vcpkg
REM ----------------------------
if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    pushd "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat
    if %errorlevel% neq 0 (
        echo Failed to bootstrap vcpkg!
        popd
        pause
        exit /b %errorlevel%
    )
    popd
) else (
    echo vcpkg already bootstrapped
)

REM ----------------------------
REM Step 4: Configure with CMake
REM ----------------------------
echo Configuring project...

cmake -S . -B %BUILD_DIR% ^
    -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows

if %errorlevel% neq 0 (
    echo CMake configure failed!
    pause
    exit /b %errorlevel%
)

REM ----------------------------
REM Step 5: Build
REM ----------------------------
echo Building project...

cmake --build %BUILD_DIR% --config Debug

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo.
echo SUCCESS
pause