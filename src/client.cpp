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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9001
#define BUFFER_SIZE 1024

// clientside messaging
class SecureClient {
private:
    SOCKET socket_fd;
    MemArena arena;
    CryptoEngine crypto_engine;
    MessageLogger logger;
    KeyPair my_keypair;
    KeyPair peer_keypair;
    std::string my_name;
    bool should_exit;

public:
    SecureClient() : socket_fd(INVALID_SOCKET), arena(10 * 1024 * 1024), crypto_engine(), 
                    logger("logs/messages.txt"), my_name("Client"), should_exit(false) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            std::cerr << "[Client] WSAStartup failed!" << std::endl;
            throw std::runtime_error("WSAStartup failed");
        }

        // gen keys
        my_keypair.generate(arena);
        std::cout << "[Client] Generated keypair" << std::endl;
    }

    ~SecureClient() {
        if (socket_fd != INVALID_SOCKET) {
            closesocket(socket_fd);
        }
        WSACleanup();
    }

    bool connect_to_server() {
        socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_fd == INVALID_SOCKET) {
            std::cerr << "[Client] Socket creation failed!" << std::endl;
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(SERVER_PORT);

        std::cout << "\n========================================" << std::endl;
        std::cout << "  Secure Messaging Client" << std::endl;
        std::cout << "========================================\n" << std::endl;
        std::cout << "[Client] Connecting to server at " << SERVER_IP << ":" << SERVER_PORT << "..." << std::endl;

        int max_retries = 5;
        for (int i = 0; i < max_retries; ++i) {
            if (connect(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
                std::cout << "[Client] Connected to server!" << std::endl;
                return true;
            }
            std::cout << "[Client] Retrying connection (" << (i + 1) << "/" << max_retries << ")..." << std::endl;
            Sleep(1000);
        }

        std::cerr << "[Client] Failed to connect to server!" << std::endl;
        closesocket(socket_fd);
        socket_fd = INVALID_SOCKET;
        return false;
    }

    bool exchange_keypairs() {
        // receive server's public key
        ui8 received_key[32];
        int recv_len = recv(socket_fd, (char*)received_key, 32, 0);
        if (recv_len != 32) {
            std::cerr << "[Client] Failed to receive server's public key!" << std::endl;
            return false;
        }

        peer_keypair.public_key = (ui8*)arena.push(32, 0);
        memcpy(peer_keypair.public_key, received_key, 32);

        std::cout << "[Client] Received server's public key" << std::endl;

        //send my public key
        if (send(socket_fd, (const char*)my_keypair.public_key, 32, 0) == SOCKET_ERROR) {
            std::cerr << "[Client] Failed to send public key!" << std::endl;
            return false;
        }

        std::cout << "[Client] Sent public key to server" << std::endl;
        return true;
    }

    static void recv_thread_func(void* arg) {
        SecureClient* client = (SecureClient*)arg;
        char buffer[BUFFER_SIZE];
        char len_buffer[4];
        int len_received = 0;
        
        while (!client->should_exit) {
            // read message length (4 bytes)
            if (len_received < 4) {
                int recv_len = recv(client->socket_fd, len_buffer + len_received, 4 - len_received, 0);
                
                if (recv_len == 0) {
                    std::cout << "\n[Client] Server disconnected!" << std::endl;
                    client->should_exit = true;
                    break;
                } else if (recv_len == SOCKET_ERROR) {
                    int error = WSAGetLastError();
                    if (error != WSAEWOULDBLOCK && error != WSAEINTR && error != WSAECONNRESET) {
                        std::cerr << "\n[Client] Recv error: " << error << std::endl;
                        client->should_exit = true;
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
                    int recv_len = recv(client->socket_fd, buffer, msg_len, 0);
                    
                    if (recv_len > 0) {
                        buffer[recv_len] = '\0';
                        std::cout << "\n[Server] " << buffer << std::endl;
                        std::cout << "[You] ";
                        std::cout.flush();
                        
                        
                        //eeset for nxt msg
                        len_received = 0;
                    } else if (recv_len == SOCKET_ERROR) {
                        int error = WSAGetLastError();
                        if (error == WSAEWOULDBLOCK || error == WSAEINTR) {
                            //wait for continous data
                            Sleep(10);
                        } else {
                            std::cerr << "\n[Client] Message recv error: " << error << std::endl;
                            client->should_exit = true;
                            break;
                        }
                    }
                } else if (msg_len == 0) {
                    // empty message, just reset
                    len_received = 0;
                } else {
                    // invalid message length
                    std::cerr << "\n[Client] Invalid message length: " << msg_len << std::endl;
                    len_received = 0;
                }
            }
            
            Sleep(10);
        }
        _endthread();
    }

    static void send_thread_func(void* arg) {
        SecureClient* client = (SecureClient*)arg;
        std::string input_line;
        
        std::cout << "[You] ";
        std::cout.flush();
        
        while (!client->should_exit) {
            if (std::getline(std::cin, input_line)) {
                if (input_line == "exit") {
                    client->should_exit = true;
                    std::cout << "[Client] Shutting down..." << std::endl;
                    break;
                }

                if (!input_line.empty()) {
                    ui32 msg_len = (ui32)input_line.length();
                    int bytes_sent = 0;
                    
                    while (bytes_sent < (int)sizeof(ui32)) {
                        int send_result = send(client->socket_fd, (const char*)&msg_len + bytes_sent, sizeof(ui32) - bytes_sent, 0);
                        
                        if (send_result == SOCKET_ERROR) {
                            int error = WSAGetLastError();
                            std::cerr << "\n[Client] Send length failed! Error: " << error << std::endl;
                            client->should_exit = true;
                            break;
                        } else if (send_result > 0) {
                            bytes_sent += send_result;
                        } else {
                            Sleep(10);
                        }
                    }
                    
                    if (client->should_exit) break;
                    
                    bytes_sent = 0;
                    while (bytes_sent < (int)input_line.length()) {
                        int send_result = send(client->socket_fd, input_line.c_str() + bytes_sent, input_line.length() - bytes_sent, 0);
                        
                        if (send_result == SOCKET_ERROR) {
                            int error = WSAGetLastError();
                            std::cerr << "\n[Client] Send message failed! Error: " << error << std::endl;
                            client->should_exit = true;
                            break;
                        } else if (send_result > 0) {
                            bytes_sent += send_result;
                        } else {
                            Sleep(10);
                        }
                    }
                    
                    if (client->should_exit) break;
                    
                    try {
                        client->logger.log_sent_message("Client", input_line,
                                                       client->peer_keypair, client->my_keypair);
                    } catch (const std::exception& e) {
                        std::cerr << "\n[Client] Logging error: " << e.what() << std::endl;
                        std::cerr << "[Client] Message was: " << input_line << std::endl;
                    }
                }
                
                std::cout << "[You] ";
                std::cout.flush();
            } else {
                // getline failed (EOF or error)
                break;
            }
        }
        _endthread();
    }

    void run() {
        std::cout << "\n[Client] Ready to send/receive messages. Type 'exit' to quit.\n" << std::endl;

        MessageLogger::print_log_info();

        // start receive thread
        _beginthread(recv_thread_func, 0, (void*)this);
        
        // start send thread
        _beginthread(send_thread_func, 0, (void*)this);
        
        // wait for threads to finish
        while (!should_exit) {
            Sleep(100);
        }
        Sleep(500); //clean
    }
};


int main() {
    try {
        SecureClient client;

        if (!client.connect_to_server()) {
            return 1;
        }

        if (!client.exchange_keypairs()) {
            return 1;
        }

        client.run();

    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
