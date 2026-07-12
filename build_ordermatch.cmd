@echo off
setlocal enabledelayedexpansion
echo ========================================
echo   Building OrderMatchBackend
echo ========================================

where cl.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo Initializing MSVC environment...
    set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!vswhere!" (
        for /f "usebackq tokens=*" %%i in (`"!vswhere!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set "InstallDir=%%i"
        )
        if exist "!InstallDir!\VC\Auxiliary\Build\vcvars64.bat" (
            call "!InstallDir!\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
    where cl.exe >nul 2>nul
    if !errorlevel! neq 0 (
        echo Error: Could not find MSVC compiler. Please run this script from a Visual Studio Developer Command Prompt.
        exit /b 1
    )
)

set BUILD_DIR=build_ordermatch
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo Configuring CMake...
cmake ../examples/OrderMatchBackend
if %errorlevel% equ 0 goto build_step

echo CMake configuration failed! Cleaning cache and retrying...
if exist CMakeCache.txt del CMakeCache.txt
cmake ../examples/OrderMatchBackend
if %errorlevel% neq 0 (
    echo CMake configuration failed again!
    cd ..
    exit /b %errorlevel%
)

:build_step

echo.
echo Building Project...
cmake --build .
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b %errorlevel%
)

echo.
echo Build completed successfully!
cd ..
