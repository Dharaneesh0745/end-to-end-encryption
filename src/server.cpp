#include <iostream>
#include <winsock2.h>
#include <string>
#include <sstream>
#include <process.h>
#include <cstdlib>
#include "../include/crypto.h"
#include "../include/arena.h"
#include "../include/message.h"
#include "../include/logger.h"

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#define PORT 9001
#define BUFFER_SIZE 1024

//server side messaging
class SecureServer {
private:
    SOCKET server_socket;
    SOCKET client_socket;
    MemArena arena;
    CryptoEngine crypto_engine;
    MessageLogger logger;
    KeyPair my_keypair;
    KeyPair peer_keypair;
    std::string my_name;
    bool should_exit;

public:
    SecureServer() : server_socket(INVALID_SOCKET), client_socket(INVALID_SOCKET), 
                    arena(10 * 1024 * 1024), crypto_engine(), logger("logs/messages.txt"), my_name("Server"), should_exit(false) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            std::cerr << "[Server] WSAStartup failed!" << std::endl;
            throw std::runtime_error("WSAStartup failed");
        }

        // gen keys
        my_keypair.generate(arena);
        std::cout << "[Server] Generated keypair" << std::endl;
    }

    ~SecureServer() {
        if (client_socket != INVALID_SOCKET) {
            closesocket(client_socket);
        }
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
        }
        WSACleanup();
    }

    bool start() {
        server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "[Server] Socket creation failed!" << std::endl;
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(PORT);

        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "[Server] Bind failed!" << std::endl;
            closesocket(server_socket);
            return false;
        }

        if (listen(server_socket, 1) == SOCKET_ERROR) {
            std::cerr << "[Server] Listen failed!" << std::endl;
            closesocket(server_socket);
            return false;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "  Secure Messaging Server" << std::endl;
        std::cout << "========================================\n" << std::endl;
        std::cout << "[Server] Listening on port " << PORT << "..." << std::endl;

        return true;
    }

    bool accept_client() {
        sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);

        client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "[Server] Accept failed!" << std::endl;
            return false;
        }

        std::cout << "[Server] Client connected!" << std::endl;

        //exchange keypairs
        if (!exchange_keypairs()) {
            std::cerr << "[Server] Keypair exchange failed!" << std::endl;
            return false;
        }

        MessageLogger::print_log_info();
        return true;
    }

    bool exchange_keypairs() {
        // send my public key to client
        if (send(client_socket, (const char*)my_keypair.public_key, 32, 0) == SOCKET_ERROR) {
            std::cerr << "[Server] Failed to send public key!" << std::endl;
            return false;
        }

        std::cout << "[Server] Sent public key to client" << std::endl;

        // recv--client--public--key
        ui8 received_key[32];
        int recv_len = recv(client_socket, (char*)received_key, 32, 0);
        if (recv_len != 32) {
            std::cerr << "[Server] Failed to receive public key!" << std::endl;
            return false;
        }

        peer_keypair.public_key = (ui8*)arena.push(32, 0);
        memcpy(peer_keypair.public_key, received_key, 32);

        std::cout << "[Server] Received client's public key" << std::endl;
        return true;
    }

    static void recv_thread_func(void* arg) {
        SecureServer* server = (SecureServer*)arg;
        char buffer[BUFFER_SIZE];
        char len_buffer[4];
        int len_received = 0;
        
        while (!server->should_exit) {
            // read message length (4 bytes)
            if (len_received < 4) {
                int recv_len = recv(server->client_socket, len_buffer + len_received, 4 - len_received, 0);
                
                if (recv_len == 0) {
                    std::cout << "\n[Server] Client disconnected!" << std::endl;
                    server->should_exit = true;
                    break;
                } else if (recv_len == SOCKET_ERROR) {
                    int error = WSAGetLastError();
                    if (error != WSAEWOULDBLOCK && error != WSAEINTR && error != WSAECONNRESET) {
                        std::cerr << "\n[Server] Recv error: " << error << std::endl;
                        server->should_exit = true;
                        break;
                    }
                    Sleep(10);
                    continue;
                } else if (recv_len > 0) {
                    len_received += recv_len;
                }
            }
            
            if (len_received == 4) {
                ui32 msg_len = *(ui32*)len_buffer;
                
                if (msg_len > 0 && msg_len < BUFFER_SIZE) {
                    // read the message content
                    int recv_len = recv(server->client_socket, buffer, msg_len, 0);
                    
                    if (recv_len > 0) {
                        buffer[recv_len] = '\0';
                        std::cout << "\n[Client] " << buffer << std::endl;
                        std::cout << "[You] ";
                        std::cout.flush();
                        
                        len_received = 0;
                    } else if (recv_len == SOCKET_ERROR) {
                        int error = WSAGetLastError();
                        if (error == WSAEWOULDBLOCK || error == WSAEINTR) {
                            // Just wait for more data
                            Sleep(10);
                        } else {
                            std::cerr << "\n[Server] Message recv error: " << error << std::endl;
                            server->should_exit = true;
                            break;
                        }
                    }
                } else if (msg_len == 0) {
                    len_received = 0;
                } else {
                    std::cerr << "\n[Server] Invalid message length: " << msg_len << std::endl;
                    len_received = 0;
                }
            }
            
            Sleep(10);
        }
        _endthread();
    }

    static void send_thread_func(void* arg) {
        SecureServer* server = (SecureServer*)arg;
        std::string input_line;
        
        std::cout << "[You] ";
        std::cout.flush();
        
        while (!server->should_exit) {
            if (std::getline(std::cin, input_line)) {
                if (input_line == "exit") {
                    server->should_exit = true;
                    std::cout << "[Server] Shutting down..." << std::endl;
                    break;
                }

                if (!input_line.empty()) {
                    ui32 msg_len = (ui32)input_line.length();
                    int bytes_sent = 0;
                    
                    // keep sending until all length bytes are sent
                    while (bytes_sent < (int)sizeof(ui32)) {
                        int send_result = send(server->client_socket, (const char*)&msg_len + bytes_sent, sizeof(ui32) - bytes_sent, 0);
                        
                        if (send_result == SOCKET_ERROR) {
                            int error = WSAGetLastError();
                            std::cerr << "\n[Server] Send length failed! Error: " << error << std::endl;
                            server->should_exit = true;
                            break;
                        } else if (send_result > 0) {
                            bytes_sent += send_result;
                        } else {
                            Sleep(10);
                        }
                    }
                    
                    if (server->should_exit) break;
                    
                    // send message content
                    bytes_sent = 0;
                    while (bytes_sent < (int)input_line.length()) {
                        int send_result = send(server->client_socket, input_line.c_str() + bytes_sent, input_line.length() - bytes_sent, 0);
                        
                        if (send_result == SOCKET_ERROR) {
                            int error = WSAGetLastError();
                            std::cerr << "\n[Server] Send message failed! Error: " << error << std::endl;
                            server->should_exit = true;
                            break;
                        } else if (send_result > 0) {
                            bytes_sent += send_result;
                        } else {
                            Sleep(10);
                        }
                    }
                    
                    if (server->should_exit) break;
                    
                    try {
                        server->logger.log_sent_message("Server", input_line,
                                                       server->peer_keypair, server->my_keypair);
                    } catch (const std::exception& e) {
                        std::cerr << "\n[Server] Logging error: " << e.what() << std::endl;
                        std::cerr << "[Server] Message was: " << input_line << std::endl;
                    }
                }
                
                std::cout << "[You] ";
                std::cout.flush();
            } else {
                break;
            }
        }
        _endthread();
    }

    void run() {
        std::cout << "\n[Server] Ready to send/receive messages. Type 'exit' to quit.\n" << std::endl;
        _beginthread(recv_thread_func, 0, (void*)this);
        _beginthread(send_thread_func, 0, (void*)this);
        while (!should_exit) {
            Sleep(100);
        }
        Sleep(500);
    }
};

int main() {
    try {
        SecureServer server;

        if (!server.start()) {
            return 1;
        }

        if (!server.accept_client()) {
            return 1;
        }

        server.run();

    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
