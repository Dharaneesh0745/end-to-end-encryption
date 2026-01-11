@echo off
REM 

echo ========================================
echo Secure Messaging System - Build Script
echo ========================================
echo.

REM 
if not exist "logs" (
    mkdir logs
    echo Created logs directory
)

REM 
echo.

echo [1/1] Building Combined Binary...
g++ -std=c++17 -Wall -I./include -o main_combined.exe src/main_combined.cpp src/server.cpp src/client.cpp src/crypto.cpp -lws2_32
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build main_combined!
    exit /b 1
)
echo Combined binary built successfully: main_combined.exe

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo To run the system:
echo   1. Open first CMD window and run: server.exe
echo   2. Open second CMD window and run: client.exe
echo.
echo All conversations will be logged to: logs/messages.txt
echo.
