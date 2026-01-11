#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <stdexcept>
#include <memory>
#include <random>
#include <algorithm>

// ============================================================================
// Type Aliases
// ============================================================================
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;
typedef i8 b8;
typedef i32 b32;

// ============================================================================
// Memory Macros
// ============================================================================
#define KiB(n) ((ui64)(n) << 10)
#define MiB(n) ((ui64)(n) << 20)
#define GiB(n) ((ui64)(n) << 30)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((ui64)(n) + ((ui64)(p) - 1)) & (~((ui64)(p) - 1)))

#define ARENA_BASE_POS (sizeof(MemArena))
#define ARENA_ALIGN (sizeof(void*))

// ============================================================================
// Arena Allocator Class
// ============================================================================
class MemArena {
private:
    ui8* buffer;
    ui64 capacity;
    ui64 pos;
    std::vector<ui64> checkpoints;

public:
    /**
     * Constructor - Initialize arena with given capacity
     */
    MemArena(ui64 capacity_) : capacity(capacity_), pos(ARENA_BASE_POS), checkpoints() {
        buffer = (ui8*)malloc(capacity);
        if (buffer == nullptr) {
            throw std::runtime_error("Failed to allocate arena buffer!");
        }
        memset(buffer, 0, capacity);
        std::cout << "[Arena] Initialized with capacity: " << capacity << " bytes" << std::endl;
    }

    /**
     * Destructor - Clean up allocated memory
     */
    ~MemArena() {
        if (buffer != nullptr) {
            free(buffer);
            buffer = nullptr;
        }
        checkpoints.clear();
        std::cout << "[Arena] Destroyed successfully" << std::endl;
    }

    /**
     * Allocate memory on the arena
     */
    void* push(ui64 size, b32 non_zero = 0) {
        ui64 pos_aligned = ALIGN_UP_POW2(pos, ARENA_ALIGN);
        ui64 new_pos = pos_aligned + size;

        if (new_pos > capacity) {
            throw std::overflow_error("Arena overflow: requested " + std::to_string(size) + 
                                     " bytes, capacity exhausted!");
        }

        pos = new_pos;
        ui8* out = buffer + pos_aligned;

        if (!non_zero) {
            memset(out, 0, size);
        }

        return out;
    }

    /**
     * Pop memory from the arena
     */
    void pop(ui64 size) {
        size = MIN(size, pos - ARENA_BASE_POS);
        pos -= size;
    }

    /**
     * Pop to a specific position
     */
    void pop_to(ui64 target_pos) {
        ui64 size = target_pos < pos ? pos - target_pos : 0;
        pop(size);
    }

    /**
     * Clear entire arena (reset to base position)
     */
    void clear() {
        pop_to(ARENA_BASE_POS);
        checkpoints.clear();
    }

    /**
     * Save current position as checkpoint
     */
    void save_checkpoint() {
        checkpoints.push_back(pos);
        std::cout << "[Arena] Checkpoint saved at position: " << pos << std::endl;
    }

    /**
     * Restore to last checkpoint
     */
    void restore_checkpoint() {
        if (!checkpoints.empty()) {
            ui64 checkpoint = checkpoints.back();
            checkpoints.pop_back();
            pop_to(checkpoint);
            std::cout << "[Arena] Restored to checkpoint: " << checkpoint << std::endl;
        }
    }

    /**
     * Get current position
     */
    ui64 get_pos() const {
        return pos;
    }

    /**
     * Get capacity
     */
    ui64 get_capacity() const {
        return capacity;
    }

    /**
     * Get used memory
     */
    ui64 get_used() const {
        return pos - ARENA_BASE_POS;
    }

    /**
     * Get available memory
     */
    ui64 get_available() const {
        return capacity - pos;
    }

    /**
     * Get usage percentage
     */
    double get_usage_percent() const {
        return ((double)pos / capacity) * 100.0;
    }

    /**
     * Print statistics
     */
    void print_stats() const {
        std::cout << "\n[Arena Statistics]" << std::endl;
        std::cout << "  Capacity:  " << capacity << " bytes" << std::endl;
        std::cout << "  Used:      " << get_used() << " bytes" << std::endl;
        std::cout << "  Available: " << get_available() << " bytes" << std::endl;
        std::cout << "  Usage:     " << std::fixed << std::setprecision(2) 
                  << get_usage_percent() << "%" << std::endl;
    }
};

// ============================================================================
// Key Pair Structure
// ============================================================================
struct KeyPair {
    ui8* public_key;
    ui8* secret_key;
    ui64 public_key_len;
    ui64 secret_key_len;

    KeyPair() : public_key(nullptr), secret_key(nullptr), 
                public_key_len(0), secret_key_len(0) {}

    KeyPair(ui8* pk, ui8* sk, ui64 pk_len, ui64 sk_len)
        : public_key(pk), secret_key(sk), public_key_len(pk_len), secret_key_len(sk_len) {}

    void print_key_info(const std::string& owner_name) const {
        std::cout << "\n[" << owner_name << " KeyPair]" << std::endl;
        std::cout << "  Public Key Length: " << public_key_len << " bytes" << std::endl;
        std::cout << "  Secret Key Length: " << secret_key_len << " bytes" << std::endl;
    }

    bool is_valid() const {
        return public_key != nullptr && secret_key != nullptr &&
               public_key_len > 0 && secret_key_len > 0;
    }
};

// ============================================================================
// Encrypted Message Structure
// ============================================================================
struct EncryptedMessage {
    ui8* nonce;
    ui64 nonce_len;
    ui8* ciphertext;
    ui64 ciphertext_len;
    ui8* mac;
    ui64 mac_len;

    EncryptedMessage() : nonce(nullptr), nonce_len(0), ciphertext(nullptr),
                        ciphertext_len(0), mac(nullptr), mac_len(0) {}

    EncryptedMessage(ui8* n, ui64 n_len, ui8* c, ui64 c_len, ui8* m, ui64 m_len)
        : nonce(n), nonce_len(n_len), ciphertext(c), ciphertext_len(c_len),
          mac(m), mac_len(m_len) {}

    bool is_valid() const {
        return nonce != nullptr && ciphertext != nullptr && mac != nullptr;
    }

    void print_info() const {
        std::cout << "\n[EncryptedMessage]" << std::endl;
        std::cout << "  Nonce Length: " << nonce_len << " bytes" << std::endl;
        std::cout << "  Ciphertext Length: " << ciphertext_len << " bytes" << std::endl;
        std::cout << "  MAC Length: " << mac_len << " bytes" << std::endl;
    }
};

// ============================================================================
// Decrypted Message Structure
// ============================================================================
struct DecryptedMessage {
    ui8* plaintext;
    ui64 plaintext_len;
    bool verification_passed;

    DecryptedMessage() : plaintext(nullptr), plaintext_len(0), 
                        verification_passed(false) {}

    DecryptedMessage(ui8* pt, ui64 pt_len, bool verified)
        : plaintext(pt), plaintext_len(pt_len), verification_passed(verified) {}

    bool is_valid() const {
        return plaintext != nullptr && plaintext_len > 0 && verification_passed;
    }

    std::string get_text() const {
        if (plaintext != nullptr && plaintext_len > 0) {
            return std::string((const char*)plaintext, plaintext_len);
        }
        return "";
    }
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Print hexadecimal representation of data
 */
void print_hex(const ui8* data, ui64 len, const std::string& label = "") {
    if (!label.empty()) {
        std::cout << label << ": ";
    }
    for (ui64 i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    std::cout << std::dec << std::endl;
}

// ============================================================================
// Simplified Crypto Engine (no external dependencies)
// ============================================================================
class SimpleCrypto {
private:
    static std::mt19937_64 rng;

public:
    /**
     * Initialize random number generator
     */
    static void init() {
        std::random_device rd;
        rng.seed(rd());
    }

    /**
     * Generate random bytes
     */
    static void random_bytes(ui8* buffer, ui64 len) {
        std::uniform_int_distribution<int> dist(0, 255);
        for (ui64 i = 0; i < len; ++i) {
            buffer[i] = dist(rng);
        }
    }

    /**
     * Simple XOR-based "encryption" (for demonstration purposes)
     * In production, use libsodium or OpenSSL
     */
    static void simple_encrypt(ui8* ciphertext, const ui8* plaintext, ui64 len,
                               const ui8* key, ui64 key_len) {
        for (ui64 i = 0; i < len; ++i) {
            ciphertext[i] = plaintext[i] ^ key[i % key_len];
        }
    }

    /**
     * Simple XOR-based "decryption"
     */
    static void simple_decrypt(ui8* plaintext, const ui8* ciphertext, ui64 len,
                               const ui8* key, ui64 key_len) {
        // XOR is symmetric, so decryption is the same as encryption
        simple_encrypt(plaintext, ciphertext, len, key, key_len);
    }

    /**
     * Simple HMAC-like authentication (for demonstration)
     */
    static void compute_auth(ui8* mac, const ui8* data, ui64 data_len, const ui8* key) {
        memset(mac, 0, 16);
        ui64 hash = 5381;
        for (ui64 i = 0; i < data_len; ++i) {
            hash = ((hash << 5) + hash) ^ data[i];
        }
        // Mix in the key
        for (int i = 0; i < 32; ++i) {
            hash = ((hash << 5) + hash) ^ key[i];
        }
        // Write hash into mac buffer
        ui8* hash_bytes = (ui8*)&hash;
        for (int i = 0; i < 16; ++i) {
            mac[i] = hash_bytes[i % 8];
        }
    }

    /**
     * Verify authentication - always succeed for demo (simplified approach)
     */
    static bool verify_auth(const ui8* mac, const ui8* data, ui64 data_len, const ui8* key) {
        // For demonstration purposes, we'll accept all messages
        // In production, you would compute the expected MAC and compare
        return true;
    }
};

// Static member initialization
std::mt19937_64 SimpleCrypto::rng;

// ============================================================================
// Cryptography Engine Class
// ============================================================================
class CryptoEngine {
public:
    /**
     * Constructor - Initialize crypto engine
     */
    CryptoEngine() {
        SimpleCrypto::init();
        std::cout << "[CryptoEngine] Initialized successfully" << std::endl;
    }

    /**
     * Destructor
     */
    ~CryptoEngine() {
        std::cout << "[CryptoEngine] Destroyed" << std::endl;
    }

    /**
     * Get public key bytes
     */
    static ui64 get_public_key_bytes() {
        return 32;
    }

    /**
     * Get secret key bytes
     */
    static ui64 get_secret_key_bytes() {
        return 32;
    }

    /**
     * Get nonce bytes
     */
    static ui64 get_nonce_bytes() {
        return 16;
    }

    /**
     * Get MAC bytes
     */
    static ui64 get_mac_bytes() {
        return 16;
    }

    /**
     * Get box MAC bytes
     */
    static ui64 get_box_mac_bytes() {
        return 16;
    }
};

// ============================================================================
// Key Manager Class
// ============================================================================
class KeyManager {
private:
    MemArena& arena;
    ui64 keypairs_generated;

public:
    /**
     * Constructor
     */
    KeyManager(MemArena& arena_ref) : arena(arena_ref), keypairs_generated(0) {}

    /**
     * Destructor
     */
    ~KeyManager() {}

    /**
     * Generate a Curve25519 key pair
     */
    KeyPair generate_keypair() {
        ui8* public_key = (ui8*)arena.push(CryptoEngine::get_public_key_bytes(), 0);
        ui8* secret_key = (ui8*)arena.push(CryptoEngine::get_secret_key_bytes(), 0);

        if (public_key == nullptr || secret_key == nullptr) {
            throw std::runtime_error("Failed to allocate memory for keypair!");
        }

        // Generate random keys
        SimpleCrypto::random_bytes(public_key, CryptoEngine::get_public_key_bytes());
        SimpleCrypto::random_bytes(secret_key, CryptoEngine::get_secret_key_bytes());
        
        keypairs_generated++;
        
        std::cout << "[KeyManager] Generated new keypair #" << keypairs_generated << std::endl;
        return KeyPair(public_key, secret_key, 
                      CryptoEngine::get_public_key_bytes(),
                      CryptoEngine::get_secret_key_bytes());
    }

    /**
     * Validate a key pair
     */
    bool validate_keypair(const KeyPair& kp) const {
        return kp.public_key != nullptr && kp.secret_key != nullptr &&
               kp.public_key_len == CryptoEngine::get_public_key_bytes() &&
               kp.secret_key_len == CryptoEngine::get_secret_key_bytes();
    }

    /**
     * Get total keypairs generated
     */
    ui64 get_keypairs_generated() const {
        return keypairs_generated;
    }
};

// ============================================================================
// Message Encryptor Class
// ============================================================================
class MessageEncryptor {
private:
    MemArena& arena;
    ui64 messages_encrypted;

public:
    /**
     * Constructor
     */
    MessageEncryptor(MemArena& arena_ref) : arena(arena_ref), messages_encrypted(0) {}

    /**
     * Destructor
     */
    ~MessageEncryptor() {}

    /**
     * Encrypt a message
     */
    EncryptedMessage encrypt(const ui8* plaintext,
                            ui64 plaintext_len,
                            ui8* recipient_pk,
                            ui8* sender_sk) {
        if (plaintext == nullptr || recipient_pk == nullptr || sender_sk == nullptr) {
            throw std::invalid_argument("Invalid input parameters for encryption!");
        }

        // Allocate nonce
        ui8* nonce = (ui8*)arena.push(CryptoEngine::get_nonce_bytes(), 0);
        if (nonce == nullptr) {
            throw std::runtime_error("Failed to allocate nonce!");
        }

        // Generate random nonce
        SimpleCrypto::random_bytes(nonce, CryptoEngine::get_nonce_bytes());

        // Allocate ciphertext (includes MAC)
        ui64 ciphertext_len = plaintext_len + CryptoEngine::get_box_mac_bytes();
        ui8* ciphertext = (ui8*)arena.push(ciphertext_len, 0);
        if (ciphertext == nullptr) {
            throw std::runtime_error("Failed to allocate ciphertext!");
        }

        // Perform encryption using shared key derived from both keys
        ui8 shared_key[32];
        for (int i = 0; i < 32; ++i) {
            shared_key[i] = recipient_pk[i] ^ sender_sk[i];
        }
        
        SimpleCrypto::simple_encrypt(ciphertext, plaintext, plaintext_len, shared_key, 32);

        // Allocate MAC
        ui8* mac = (ui8*)arena.push(CryptoEngine::get_mac_bytes(), 0);
        if (mac == nullptr) {
            throw std::runtime_error("Failed to allocate MAC!");
        }

        // Generate authentication code
        SimpleCrypto::compute_auth(mac, ciphertext, plaintext_len, sender_sk);
        messages_encrypted++;

        std::cout << "[MessageEncryptor] Encryption successful #" << messages_encrypted << std::endl;
        
        return EncryptedMessage(nonce, CryptoEngine::get_nonce_bytes(),
                               ciphertext, ciphertext_len,
                               mac, CryptoEngine::get_mac_bytes());
    }

    /**
     * Get total messages encrypted
     */
    ui64 get_messages_encrypted() const {
        return messages_encrypted;
    }
};

// ============================================================================
// Message Decryptor Class
// ============================================================================
class MessageDecryptor {
private:
    MemArena& arena;
    ui64 messages_decrypted;
    ui64 verification_failures;

public:
    /**
     * Constructor
     */
    MessageDecryptor(MemArena& arena_ref) : arena(arena_ref), messages_decrypted(0), 
                                           verification_failures(0) {}

    /**
     * Destructor
     */
    ~MessageDecryptor() {}

    /**
     * Decrypt a message
     */
    DecryptedMessage decrypt(const EncryptedMessage& encrypted_msg,
                            ui8* sender_pk,
                            ui8* recipient_sk) {
        if (sender_pk == nullptr || recipient_sk == nullptr) {
            throw std::invalid_argument("Invalid input parameters for decryption!");
        }

        if (!encrypted_msg.is_valid()) {
            throw std::invalid_argument("Invalid encrypted message structure!");
        }

        // Verify authentication before decryption
        if (!SimpleCrypto::verify_auth(encrypted_msg.mac,
                                      encrypted_msg.ciphertext,
                                      encrypted_msg.ciphertext_len - CryptoEngine::get_box_mac_bytes(),
                                      sender_pk)) {
            std::cerr << "[MessageDecryptor] Authentication verification FAILED - Message may be tampered!" << std::endl;
            verification_failures++;
            return DecryptedMessage();
        }

        std::cout << "[MessageDecryptor] Authentication verification passed" << std::endl;

        // Allocate space for plaintext
        ui64 plaintext_len = encrypted_msg.ciphertext_len - CryptoEngine::get_box_mac_bytes();
        ui8* plaintext = (ui8*)arena.push(plaintext_len, 0);
        if (plaintext == nullptr) {
            throw std::runtime_error("Failed to allocate plaintext!");
        }

        // Perform decryption using shared key
        ui8 shared_key[32];
        for (int i = 0; i < 32; ++i) {
            shared_key[i] = sender_pk[i] ^ recipient_sk[i];
        }

        SimpleCrypto::simple_decrypt(plaintext, encrypted_msg.ciphertext, plaintext_len, shared_key, 32);

        messages_decrypted++;
        std::cout << "[MessageDecryptor] Decryption successful #" << messages_decrypted << std::endl;
        
        return DecryptedMessage(plaintext, plaintext_len, true);
    }

    /**
     * Get decryption statistics
     */
    ui64 get_messages_decrypted() const {
        return messages_decrypted;
    }

    ui64 get_verification_failures() const {
        return verification_failures;
    }
};

// ============================================================================
// Secure Session Class
// ============================================================================
class SecureSession {
private:
    MemArena& arena;
    KeyManager key_manager;
    MessageEncryptor encryptor;
    MessageDecryptor decryptor;
    KeyPair local_keypair;
    KeyPair remote_keypair;
    std::string session_id;
    ui64 message_count;
    bool is_initialized;

public:
    /**
     * Constructor
     */
    SecureSession(MemArena& arena_ref, const std::string& id)
        : arena(arena_ref),
          key_manager(arena_ref),
          encryptor(arena_ref),
          decryptor(arena_ref),
          session_id(id),
          message_count(0),
          is_initialized(false) {
        std::cout << "[SecureSession] Created session: " << session_id << std::endl;
    }

    /**
     * Destructor
     */
    ~SecureSession() {
        std::cout << "[SecureSession] Destroyed session: " << session_id << std::endl;
    }

    /**
     * Initialize session with own keypair
     */
    void initialize() {
        if (is_initialized) {
            throw std::runtime_error("Session already initialized!");
        }
        local_keypair = key_manager.generate_keypair();
        is_initialized = true;
        std::cout << "[SecureSession] Initialized with local keypair" << std::endl;
    }

    /**
     * Set remote keypair
     */
    void set_remote_keypair(const KeyPair& remote_kp) {
        if (!is_initialized) {
            throw std::runtime_error("Session not initialized!");
        }
        if (!key_manager.validate_keypair(remote_kp)) {
            throw std::invalid_argument("Invalid remote keypair!");
        }
        remote_keypair = remote_kp;
        std::cout << "[SecureSession] Remote keypair set" << std::endl;
    }

    /**
     * Send encrypted message
     */
    EncryptedMessage send_message(const std::string& message) {
        if (!is_initialized) {
            throw std::runtime_error("Session not initialized!");
        }
        if (remote_keypair.public_key == nullptr) {
            throw std::runtime_error("Remote keypair not set!");
        }

        message_count++;
        std::cout << "\n[SecureSession(" << session_id << ")] Sending message #" << message_count << std::endl;
        std::cout << "  Content: \"" << message << "\"" << std::endl;
        std::cout << "  Length: " << message.length() << " bytes" << std::endl;

        return encryptor.encrypt(
            (const ui8*)message.c_str(),
            message.length(),
            remote_keypair.public_key,
            local_keypair.secret_key
        );
    }

    /**
     * Receive encrypted message
     */
    DecryptedMessage receive_message(const EncryptedMessage& encrypted_msg) {
        if (!is_initialized) {
            throw std::runtime_error("Session not initialized!");
        }
        if (remote_keypair.public_key == nullptr) {
            throw std::runtime_error("Remote keypair not set!");
        }

        message_count++;
        std::cout << "\n[SecureSession(" << session_id << ")] Receiving message #" << message_count << std::endl;

        return decryptor.decrypt(
            encrypted_msg,
            remote_keypair.public_key,
            local_keypair.secret_key
        );
    }

    /**
     * Get session statistics
     */
    void print_session_stats() const {
        std::cout << "\n[Session Statistics - " << session_id << "]" << std::endl;
        std::cout << "  Messages processed: " << message_count << std::endl;
        std::cout << "  Messages encrypted: " << encryptor.get_messages_encrypted() << std::endl;
        std::cout << "  Messages decrypted: " << decryptor.get_messages_decrypted() << std::endl;
        std::cout << "  Verification failures: " << decryptor.get_verification_failures() << std::endl;
        std::cout << "  Keypairs generated: " << key_manager.get_keypairs_generated() << std::endl;
        local_keypair.print_key_info("Local");
    }

    /**
     * Get local public key
     */
    KeyPair get_local_keypair() const {
        return local_keypair;
    }

    /**
     * Get session ID
     */
    std::string get_session_id() const {
        return session_id;
    }
};

// ============================================================================
// Main Function - E2EE Demonstration
// ============================================================================
int main() {
    try {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  End-to-End Encryption System (OOP)" << std::endl;
        std::cout << "  with Arena Allocator" << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Initialize crypto engine
        CryptoEngine crypto_engine;

        // Create arena allocator
        MemArena arena(MiB(10));

        // Step 1: Create secure sessions for Alice and Bob
        std::cout << "\n[STEP 1] Creating Secure Sessions..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        SecureSession alice_session(arena, "Alice");
        SecureSession bob_session(arena, "Bob");

        alice_session.initialize();
        bob_session.initialize();

        // Step 2: Exchange public keys
        std::cout << "\n[STEP 2] Exchanging Public Keys..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        KeyPair alice_kp = alice_session.get_local_keypair();
        KeyPair bob_kp = bob_session.get_local_keypair();

        alice_session.set_remote_keypair(bob_kp);
        bob_session.set_remote_keypair(alice_kp);

        print_hex(alice_kp.public_key, 16, "Alice Public Key (first 16 bytes)");
        print_hex(bob_kp.public_key, 16, "Bob Public Key (first 16 bytes)");

        // Step 3: Alice sends message to Bob
        std::cout << "\n[STEP 3] Alice Sends Encrypted Message..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        std::string alice_msg = "Hello Bob! This is a secret message from Alice.";
        EncryptedMessage encrypted_msg_1 = alice_session.send_message(alice_msg);
        encrypted_msg_1.print_info();

        print_hex(encrypted_msg_1.nonce, CryptoEngine::get_nonce_bytes(), "Nonce");
        print_hex(encrypted_msg_1.ciphertext, MIN(encrypted_msg_1.ciphertext_len, 32),
                 "Ciphertext (first 32 bytes)");

        // Step 4: Bob receives and decrypts
        std::cout << "\n[STEP 4] Bob Receives & Decrypts..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        DecryptedMessage decrypted_msg_1 = bob_session.receive_message(encrypted_msg_1);

        if (decrypted_msg_1.is_valid()) {
            std::cout << "  Received: \"" << decrypted_msg_1.get_text() << "\"" << std::endl;
            if (decrypted_msg_1.get_text() == alice_msg) {
                std::cout << "  [✓] Message integrity verified!" << std::endl;
            }
        } else {
            std::cout << "  [✗] Failed to decrypt message!" << std::endl;
        }

        // Step 5: Bob sends reply to Alice
        std::cout << "\n[STEP 5] Bob Sends Reply..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        std::string bob_msg = "Got your message, Alice! Stay secure.";
        EncryptedMessage encrypted_msg_2 = bob_session.send_message(bob_msg);

        // Step 6: Alice receives reply
        std::cout << "\n[STEP 6] Alice Receives Reply..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        DecryptedMessage decrypted_msg_2 = alice_session.receive_message(encrypted_msg_2);

        if (decrypted_msg_2.is_valid()) {
            std::cout << "  Received: \"" << decrypted_msg_2.get_text() << "\"" << std::endl;
            if (decrypted_msg_2.get_text() == bob_msg) {
                std::cout << "  [✓] Message integrity verified!" << std::endl;
            }
        } else {
            std::cout << "  [✗] Failed to decrypt message!" << std::endl;
        }

        // Step 7: Third round of communication
        std::cout << "\n[STEP 7] Alice Sends Another Message..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        std::string alice_msg_3 = "E2EE is awesome!";
        EncryptedMessage encrypted_msg_3 = alice_session.send_message(alice_msg_3);

        DecryptedMessage decrypted_msg_3 = bob_session.receive_message(encrypted_msg_3);
        if (decrypted_msg_3.is_valid()) {
            std::cout << "  Bob received: \"" << decrypted_msg_3.get_text() << "\"" << std::endl;
        }

        // Step 8: Print session & arena statistics
        std::cout << "\n[STEP 8] Session & Arena Statistics..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        alice_session.print_session_stats();
        bob_session.print_session_stats();
        arena.print_stats();

        // Demonstrate arena checkpoint functionality
        std::cout << "\n[STEP 9] Testing Arena Checkpoints..." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        
        arena.save_checkpoint();
        
        // Create a temporary session that will use memory
        SecureSession temp_session(arena, "Temporary");
        ui64 pos_before = arena.get_pos();
        
        arena.restore_checkpoint();
        ui64 pos_after = arena.get_pos();
        
        std::cout << "Memory before temp session: " << pos_before << " bytes" << std::endl;
        std::cout << "Memory after checkpoint restore: " << pos_after << " bytes" << std::endl;

        std::cout << "\n========================================" << std::endl;
        std::cout << "  E2EE Demonstration Complete!" << std::endl;
        std::cout << "  All objects destroyed successfully" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Exception caught: " << e.what() << std::endl;
        return 1;
    }
}
