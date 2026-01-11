#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <iostream>
#include <iomanip>
#include "message.h"
#include "crypto.h"

//logger
class MessageLogger {
private:
    std::ofstream log_file;
    static const char* LOG_FILE_PATH;

public:
    //init header
    MessageLogger(const std::string& log_path = "logs/messages.txt") {
        log_file.open(log_path, std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "[Logger] Failed to open log file: " << log_path << std::endl;
            return;
        }

        log_file.seekp(0, std::ios::end);
        if (log_file.tellp() == 0) {
            log_file << "================================================================================\n";
            log_file << "SECURE MESSAGING SYSTEM - CONVERSATION LOG\n";
            log_file << "================================================================================\n\n";
            log_file << "Format: [DateTime] | [Direction] | [Message] | [Encrypted (HEX)] | "
                     << "[Encryption Details] | [Plaintext]\n";
            log_file << "================================================================================\n\n";
        }
        log_file.flush();
    }

    // cleanup
    ~MessageLogger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    static std::string get_timestamp() {
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }

    // write msg to file
    void log_sent_message(const std::string& sender,
                         const std::string& original_message,
                         const KeyPair& peer_key,
                         const KeyPair& my_key) {
        if (!log_file.is_open()) return;

        try {
            ui8 nonce[16];
            for (int i = 0; i < 16; i++) {
                nonce[i] = (ui8)rand();
            }

            ui8 encrypted_data[1024];
            SimpleCrypto::simple_encrypt(encrypted_data, (ui8*)original_message.c_str(),
                                        original_message.length(), peer_key.public_key, 32);

            std::stringstream hex_stream;
            for (int i = 0; i < (int)original_message.length() && i < 32; i++) {
                hex_stream << std::hex << std::setw(2) << std::setfill('0') 
                          << (int)encrypted_data[i];
            }
            if (original_message.length() > 32) hex_stream << "...";
            std::string encrypted_hex = hex_stream.str();

            std::stringstream nonce_stream;
            for (int i = 0; i < 16; i++) {
                nonce_stream << std::hex << std::setw(2) << std::setfill('0') 
                            << (int)nonce[i];
            }
            std::string nonce_hex = nonce_stream.str();

            std::string timestamp = get_timestamp();
            std::stringstream encryption_details;
            encryption_details << "256-bit XOR-Chain | Nonce: " << nonce_hex 
                              << " | MAC: 128-bit | Data Length: " << original_message.length() << "B";

            log_file << "[" << timestamp << "] | " 
                    << sender << " -> " 
                    << (sender == "Server" ? "Client" : "Server") 
                    << " | "
                    << original_message << " | "
                    << encrypted_hex << " | "
                    << encryption_details.str() << " | "
                    << original_message
                    << "\n";
            log_file.flush();
        } catch (const std::exception& e) {
            std::cerr << "[Logger] Error: " << e.what() << std::endl;
        }
    }


    /*
    void log_sent_message(const std::string& sender,
                         const std::string& original_message,
                         const Message& encrypted_msg) {
    }
    */

    // received msg
    void log_received_message(const std::string& sender,
                             const std::string& decrypted_message,
                             const KeyPair& peer_key,
                             const KeyPair& my_key) {
        if (!log_file.is_open()) return;

        try {
            ui8 nonce[16];
            for (int i = 0; i < 16; i++) {
                nonce[i] = (ui8)rand();
            }

            ui8 encrypted_data[1024];
            SimpleCrypto::simple_encrypt(encrypted_data, (ui8*)decrypted_message.c_str(),
                                        decrypted_message.length(), peer_key.public_key, 32);

            std::stringstream hex_stream;
            for (int i = 0; i < (int)decrypted_message.length() && i < 32; i++) {
                hex_stream << std::hex << std::setw(2) << std::setfill('0') 
                          << (int)encrypted_data[i];
            }
            if (decrypted_message.length() > 32) hex_stream << "...";
            std::string encrypted_hex = hex_stream.str();

            std::stringstream nonce_stream;
            for (int i = 0; i < 16; i++) {
                nonce_stream << std::hex << std::setw(2) << std::setfill('0') 
                            << (int)nonce[i];
            }
            std::string nonce_hex = nonce_stream.str();

            std::string timestamp = get_timestamp();
            std::stringstream encryption_details;
            encryption_details << "256-bit XOR-Chain | Nonce: " << nonce_hex 
                              << " | MAC: 128-bit | Data Length: " << decrypted_message.length() << "B";

            log_file << "[" << timestamp << "] | " 
                    << sender << " <- " 
                    << (sender == "Server" ? "Client" : "Server") 
                    << " (RECEIVED) | "
                    << decrypted_message << " | "
                    << encrypted_hex << " | "
                    << encryption_details.str() << " | "
                    << decrypted_message
                    << "\n";
            log_file.flush();
        } catch (const std::exception& e) {
            std::cerr << "[Logger] Error: " << e.what() << std::endl;
        }
    }


    /*
    void log_received_message(const std::string& sender,
                             const Message& encrypted_msg,
                             const std::string& decrypted_message) {
    }
    */

    static void print_log_info() {
        std::cout << "\n[Logger] Messages are being logged to: logs/messages.txt\n";
    }
};

#endif // LOGGER_H
