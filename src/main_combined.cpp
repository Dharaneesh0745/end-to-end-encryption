// main_combined.cpp
// single binary for both server and client modes

#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

#include "../include/crypto.h"
#include "../include/arena.h"
#include "../include/message.h"
#include "../include/logger.h"

int run_server(int argc, char* argv[]);
int run_client(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: main_combined --server | --client" << std::endl;
        return 1;
    }
    std::string mode = argv[1];
    if (mode == "--server") {
        return run_server(argc, argv);
    } else if (mode == "--client") {
        return run_client(argc, argv);
    } else {
        std::cout << "invalid mode. use --server or --client" << std::endl;
        return 1;
    }
}

int run_server(int argc, char* argv[]) {
    std::cout << "server mode not yet implemented" << std::endl;
    return 0;
}

int run_client(int argc, char* argv[]) {
    std::cout << "client mode not yet implemented" << std::endl;
    return 0;
}
