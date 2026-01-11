#!/bin/sh
mkdir -p logs

echo "========================================="
echo "secure messaging system - build script"
echo "========================================="
echo

echo "[1/2] building server..."
g++ -std=c++17 -Wall -I./include -o server src/server.cpp
if [ $? -ne 0 ]; then
    echo "error: failed to build server!"
    exit 1
fi
echo "server built successfully: server"

echo

echo "[2/2] building client..."
g++ -std=c++17 -Wall -I./include -o client src/client.cpp
if [ $? -ne 0 ]; then
    echo "error: failed to build client!"
    exit 1
fi
echo "client built successfully: client"

echo

echo "========================================="
echo "build complete!"
echo "========================================="
echo

echo "to run the system:"
echo "  1. open first terminal and run: ./server"
echo "  2. open second terminal and run: ./client"
echo

echo "all conversations will be logged to: logs/messages.txt"
echo
