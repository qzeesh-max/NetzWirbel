@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   NetzWirbel Windows Build All Script
echo ========================================================
echo This script will build all native backends using MSVC
echo and all web frontends using Emscripten.
echo.

echo [1/3] Building OrderMatchBackend...
call .\build_ordermatch.cmd
if %errorlevel% neq 0 (
    echo Error building OrderMatchBackend.
    exit /b %errorlevel%
)
echo.

echo [2/3] Building OdysseyBackend...
call .\build_odyssey.cmd
if %errorlevel% neq 0 (
    echo Error building OdysseyBackend.
    exit /b %errorlevel%
)
echo.

echo [3/3] Building NetzWirbel Frontend Examples (Emscripten)...
where emcmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: emcmake not found. Please ensure Emscripten is installed and activated.
    exit /b 1
)

call emcmake cmake -S . -B build
if %errorlevel% neq 0 exit /b %errorlevel%

call cmake --build build -j 4
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo ========================================================
echo   All components built successfully!
echo ========================================================
echo.
echo Quick Start:
echo - To run the frontend orchestrator:   npm run examples
echo - To run OrderMatchBackend:           .\build_ordermatch\OrderMatchBackend.exe examples\OrderMatchBackend\ordermatch.cfg
echo - To run OdysseyBackend:              .\build_odyssey\OdysseyBackend.exe
echo.
