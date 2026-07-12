@echo off
echo Checking prerequisites...

where lcov >nul 2>nul
if %errorlevel% neq 0 (
    echo lcov not found. Please install lcov for Windows.
) else (
    echo lcov is installed.
)

where genhtml >nul 2>nul
if %errorlevel% neq 0 (
    echo genhtml not found. Please install lcov for Windows.
) else (
    echo genhtml is installed.
)

where node >nul 2>nul
if %errorlevel% neq 0 (
    echo Node.js not found. Please install Node.js.
    exit /b 1
) else (
    echo Node.js is installed.
)

where emcmake >nul 2>nul
if %errorlevel% neq 0 (
    echo emcmake not found. Please ensure Emscripten is installed and activated.
    exit /b 1
) else (
    echo Emscripten is active.
)

echo All prerequisites met.
echo Building and running tests with coverage...

call npm run test:coverage
if %errorlevel% neq 0 exit /b %errorlevel%

echo Coverage report generated successfully.
echo Starting coverage server...

call npm run coverage
