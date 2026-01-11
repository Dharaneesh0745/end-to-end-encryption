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
echo [1/2] Building Server...
g++ -std=c++17 -Wall -I./include -o server.exe src/server.cpp -lws2_32
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build server!
    exit /b 1
)
echo Server built successfully: server.exe

REM 
echo.
echo [2/2] Building Client...
g++ -std=c++17 -Wall -I./include -o client.exe src/client.cpp -lws2_32
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build client!
    exit /b 1
)
echo Client built successfully: client.exe

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
