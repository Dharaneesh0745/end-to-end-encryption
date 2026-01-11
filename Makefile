CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include
LDFLAGS = -lws2_32 -lpthread
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = .
SERVER = $(BIN_DIR)/server.exe
CLIENT = $(BIN_DIR)/client.exe
SERVER_SRC = $(SRC_DIR)/server.cpp
CLIENT_SRC = $(SRC_DIR)/client.cpp

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	@echo Building Server...
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo Server built successfully: $(SERVER)

$(CLIENT): $(CLIENT_SRC)
	@echo Building Client...
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo Client built successfully: $(CLIENT)

clean:
	@echo Cleaning up...
	@if exist $(SERVER) del /Q $(SERVER)
	@if exist $(CLIENT) del /Q $(CLIENT)
	@echo Clean complete.

run-server: $(SERVER)
	@echo Starting Server...
	@$(SERVER)

run-client: $(CLIENT)
	@echo Starting Client...
	@$(CLIENT)

help:
	@echo Available targets:
	@echo   make all        - Build both server and client
	@echo   make clean      - Remove all build artifacts
	@echo   make run-server - Build and run server
	@echo   make run-client - Build and run client
	@echo   make help       - Show this help message

.PHONY: all clean run-server run-client help
