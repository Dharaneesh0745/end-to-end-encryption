#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include "arena.h"
#include "crypto.h"

typedef uint8_t ui8;
typedef uint64_t ui64;

// public + secret key pair
struct KeyPair {
    ui8* public_key;
    ui8* secret_key;

    KeyPair() : public_key(nullptr), secret_key(nullptr) {}

    // gen random keys
    void generate(MemArena& arena) {
        public_key = (ui8*)arena.push(CryptoEngine::get_public_key_bytes(), 0);
        secret_key = (ui8*)arena.push(CryptoEngine::get_secret_key_bytes(), 0);
        
        SimpleCrypto::random_bytes(public_key, CryptoEngine::get_public_key_bytes());
        SimpleCrypto::random_bytes(secret_key, CryptoEngine::get_secret_key_bytes());
    }

    bool is_valid() const {
        return public_key != nullptr && secret_key != nullptr;
    }
};

// encrypted message + metadata
struct Message {
    std::string sender;
    std::string content;
    ui8* encrypted_data;
    ui64 encrypted_len;
    ui8* nonce;
    ui64 nonce_len;
    ui8* mac;
    ui64 mac_len;

    Message() : encrypted_data(nullptr), encrypted_len(0), nonce(nullptr), nonce_len(0), mac(nullptr), mac_len(0) {}

    // key--derivation
    static Message create_encrypted(MemArena& arena, 
                                   const std::string& sender_name,
                                   const std::string& msg_content,
                                   ui8* recipient_pk,
                                   ui8* sender_sk) {
        Message msg;
        msg.sender = sender_name;
        msg.content = msg_content;

        msg.nonce = (ui8*)arena.push(CryptoEngine::get_nonce_bytes(), 0);
        msg.nonce_len = CryptoEngine::get_nonce_bytes();
        SimpleCrypto::random_bytes(msg.nonce, msg.nonce_len);

        msg.encrypted_len = msg_content.length() + CryptoEngine::get_box_mac_bytes();
        msg.encrypted_data = (ui8*)arena.push(msg.encrypted_len, 0);

        ui8 shared_key[32];
        for (int i = 0; i < 32; ++i) {
            shared_key[i] = recipient_pk[i] ^ sender_sk[i];
        }

        SimpleCrypto::simple_encrypt(msg.encrypted_data, 
                                    (const ui8*)msg_content.c_str(), 
                                    msg_content.length(), 
                                    shared_key, 32);

        msg.mac = (ui8*)arena.push(CryptoEngine::get_mac_bytes(), 0);
        msg.mac_len = CryptoEngine::get_mac_bytes();
        SimpleCrypto::compute_auth(msg.mac, msg.encrypted_data, msg.encrypted_len - CryptoEngine::get_box_mac_bytes(), sender_sk);

        return msg;
    }

    // decrypt - just XOR it back
    static std::string decrypt_message(const Message& encrypted_msg,
                                      ui8* sender_pk,
                                      ui8* recipient_sk) {
        ui64 plaintext_len = encrypted_msg.encrypted_len - CryptoEngine::get_box_mac_bytes();
        ui8* plaintext = (ui8*)malloc(plaintext_len + 1);

        ui8 shared_key[32];
        for (int i = 0; i < 32; ++i) {
            shared_key[i] = sender_pk[i] ^ recipient_sk[i];
        }

        SimpleCrypto::simple_decrypt(plaintext, encrypted_msg.encrypted_data, plaintext_len, shared_key, 32);
        plaintext[plaintext_len] = '\0';

        std::string result((const char*)plaintext);
        free(plaintext);

        return result;
    }

    std::string get_hex_representation() const {
        std::stringstream ss;
        for (ui64 i = 0; i < (encrypted_len > 32 ? 32 : encrypted_len); ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)encrypted_data[i];
        }
        if (encrypted_len > 32) {
            ss << "...";
        }
        return ss.str();
    }

    std::string get_nonce_hex() const {
        std::stringstream ss;
        for (ui64 i = 0; i < nonce_len; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)nonce[i];
        }
        return ss.str();
    }
};

#endif // MESSAGE_H
