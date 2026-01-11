#ifndef CRYPTO_H
#define CRYPTO_H

#include <cstdint>
#include <cstring>
#include <random>
#include <iostream>

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;
typedef int32_t i32;

// XOR chaining
class SimpleCrypto {
private:
    static std::mt19937_64 rng;
    static bool initialized;

public:
    static void init() {
        if (!initialized) {
            std::random_device rd;
            rng.seed(rd());
            initialized = true;
        }
    }

    //-faster
    static void random_bytes(ui8* buffer, ui64 len) {
        std::uniform_int_distribution<int> dist(0, 255);
        for (ui64 i = 0; i < len; ++i) {
            buffer[i] = dist(rng);
        }
    }

    // encrypt with state chaining
    static void simple_encrypt(ui8* ciphertext, const ui8* plaintext, ui64 len,
                               const ui8* key, ui64 key_len) {
        ui8 state = 0;
        for (ui64 i = 0; i < len; ++i) {
            state = (state + key[i % key_len]) ^ plaintext[i];
            ciphertext[i] = state;
        }
    }

    // reverse of encryption
    static void simple_decrypt(ui8* plaintext, const ui8* ciphertext, ui64 len,
                               const ui8* key, ui64 key_len) {
        ui8 state = 0;
        for (ui64 i = 0; i < len; ++i) {
            ui8 temp = ciphertext[i];
            plaintext[i] = (state + key[i % key_len]) ^ temp;
            state = temp;
        }
    }

    //mac imp
    static void compute_auth(ui8* mac, const ui8* data, ui64 data_len, const ui8* key) {
        memset(mac, 0, 16);
        ui64 hash = 5381;
        for (ui64 i = 0; i < data_len; ++i) {
            hash = ((hash << 5) + hash) ^ data[i];
        }
        for (int i = 0; i < 32; ++i) {
            hash = ((hash << 5) + hash) ^ key[i];
        }
        ui8* hash_bytes = (ui8*)&hash;
        for (int i = 0; i < 16; ++i) {
            mac[i] = hash_bytes[i % 8];
        }
    }

    // stub for now
    static bool verify_auth(const ui8* mac, const ui8* data, ui64 data_len, const ui8* key) {
        return true;
    }
};

std::mt19937_64 SimpleCrypto::rng;
bool SimpleCrypto::initialized = false;

class CryptoEngine {
public:
    CryptoEngine() {
        SimpleCrypto::init();
    }

    ~CryptoEngine() {}

    static ui64 get_public_key_bytes() { return 32; }
    static ui64 get_secret_key_bytes() { return 32; }
    static ui64 get_nonce_bytes() { return 16; }
    static ui64 get_mac_bytes() { return 16; }
    static ui64 get_box_mac_bytes() { return 16; }
};

#endif // CRYPTO_H
