@echo off
echo Building examples...
:: Ensure the project is built before running orchestrator
call emcmake cmake -S . -B build
if %errorlevel% neq 0 exit /b %errorlevel%

call cmake --build build -j 4
if %errorlevel% neq 0 exit /b %errorlevel%

echo Starting NetzWirbel Examples Orchestrator...
echo You can access the examples in your browser (usually http://localhost:3000)
call npm run examples
