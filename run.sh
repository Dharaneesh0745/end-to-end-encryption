#!/bin/sh
mkdir -p logs

echo "========================================="
echo "secure messaging system - build script"
echo "========================================="
echo


echo "[1/1] building combined binary..."
g++ -std=c++17 -Wall -I./include -o main_combined src/main_combined.cpp src/server.cpp src/client.cpp src/crypto.cpp -lpthread
if [ $? -ne 0 ]; then
    echo "error: failed to build main_combined!"
    exit 1
fi
echo "combined binary built successfully: main_combined"

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
