#pragma once
#include <argon2.h>
#include <vector>
#include <random>
#include <string>
#include <sodium.h>
#include <fstream>

class PasswordManager {
    static const int HASH_LENGTH = 32;
    static const int SALT_LENGTH = 16;
    static const int TIME_COST = 2;
    static const int MEMORY_COST = 65536;
    static const int PARALLELISM = 1;
public:
    static std::vector<uint8_t> hashPassword(const std::string& password) {
        std::vector<uint8_t> salt(SALT_LENGTH);
        std::random_device rd;
        std::generate(salt.begin(), salt.end(), std::ref(rd));

        std::vector<uint8_t> hash(HASH_LENGTH);
        int result = argon2i_hash_raw(TIME_COST, MEMORY_COST, PARALLELISM,
            reinterpret_cast<const uint8_t*>(password.data()), password.size(),
            salt.data(), salt.size(),
            hash.data(), HASH_LENGTH);

        if (result != ARGON2_OK) {
            throw std::runtime_error("Failed to hash password");
        }

        std::vector<uint8_t> combinedSaltAndHash;
        combinedSaltAndHash.reserve(SALT_LENGTH + HASH_LENGTH);
        combinedSaltAndHash.insert(combinedSaltAndHash.end(), salt.begin(), salt.end());
        combinedSaltAndHash.insert(combinedSaltAndHash.end(), hash.begin(), hash.end());

        return combinedSaltAndHash;
    }

    static bool checkPassword(const std::string& password, const std::vector<uint8_t>& storedHash) {
        if (storedHash.size() < SALT_LENGTH + HASH_LENGTH) {
            throw std::runtime_error("Stored hash is too short");
        }

        std::vector<uint8_t> salt(storedHash.begin(), storedHash.begin() + SALT_LENGTH);
        std::vector<uint8_t> hash(storedHash.begin() + SALT_LENGTH, storedHash.end());

        std::vector<uint8_t> computedHash(HASH_LENGTH);
        int result = argon2i_hash_raw(TIME_COST, MEMORY_COST, PARALLELISM,
            reinterpret_cast<const uint8_t*>(password.data()), password.size(),
            salt.data(), salt.size(),
            computedHash.data(), HASH_LENGTH);

        if (result != ARGON2_OK) {
            throw std::runtime_error("Failed to hash password for verification");
        }

        return std::equal(computedHash.begin(), computedHash.end(), hash.begin(), hash.end());
    }

    static void encrypt(const std::string& plaintext, std::vector<unsigned char>& ciphertext, std::vector<unsigned char>& nonce, const std::vector<unsigned char>& key) {
        nonce.resize(crypto_secretbox_NONCEBYTES);
        randombytes_buf(nonce.data(), nonce.size());

        ciphertext.resize(plaintext.size() + crypto_secretbox_MACBYTES);
        int result = crypto_secretbox_easy(ciphertext.data(), reinterpret_cast<const unsigned char*>(plaintext.c_str()), plaintext.size(), nonce.data(), key.data());

        if (result != 0) {
            throw std::runtime_error("Encryption failed");
        }

        spdlog::info("Encryption successful: ciphertext size = {}", ciphertext.size());
    }

    static void decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& nonce, std::string& plaintext, const std::vector<unsigned char>& key) {
        if (nonce.size() != crypto_secretbox_NONCEBYTES) {
            throw std::runtime_error("Invalid nonce size for decryption");
        }
        if (key.size() != crypto_secretbox_KEYBYTES) {
            throw std::runtime_error("Invalid key size for decryption");
        }
        if (ciphertext.size() < crypto_secretbox_MACBYTES) {
            throw std::runtime_error("Ciphertext is too short to contain valid encrypted data");
        }

        std::vector<unsigned char> decryptedText(ciphertext.size() - crypto_secretbox_MACBYTES);
        int result = crypto_secretbox_open_easy(decryptedText.data(), ciphertext.data(), ciphertext.size(), nonce.data(), key.data());

        if (result != 0) {
            throw std::runtime_error("Decryption failed: Data is corrupt or authentication failed");
        }

        plaintext.assign(decryptedText.begin(), decryptedText.end());
        spdlog::info("Decryption successful: plaintext size = {}", plaintext.size());
    }

    static void save_to_file(const std::string& filename, const std::vector<unsigned char>& nonce, const std::vector<unsigned char>& ciphertext) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for saving");
        }

        uint32_t ciphertext_size = static_cast<uint32_t>(ciphertext.size());
        file.write(reinterpret_cast<const char*>(&ciphertext_size), sizeof(ciphertext_size));
        file.write(reinterpret_cast<const char*>(nonce.data()), nonce.size());
        file.write(reinterpret_cast<const char*>(ciphertext.data()), ciphertext.size());
    }

    static void load_from_file(const std::string& filename, std::vector<unsigned char>& nonce, std::vector<unsigned char>& ciphertext) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for loading");
        }

        uint32_t ciphertext_size;
        file.read(reinterpret_cast<char*>(&ciphertext_size), sizeof(ciphertext_size));

        nonce.resize(crypto_secretbox_NONCEBYTES);
        file.read(reinterpret_cast<char*>(nonce.data()), nonce.size());

        ciphertext.resize(ciphertext_size);
        file.read(reinterpret_cast<char*>(ciphertext.data()), ciphertext.size());
    }

    static void save_credentials(const std::string& filename, const std::string& login, const std::string& password, const std::vector<unsigned char>& key) {
        std::vector<unsigned char> nonce_login, ciphertext_login;
        encrypt(login, ciphertext_login, nonce_login, key);

        std::vector<unsigned char> nonce_password, ciphertext_password;
        encrypt(password, ciphertext_password, nonce_password, key);

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for saving credentials");
        }

        uint32_t login_size = static_cast<uint32_t>(ciphertext_login.size());
        file.write(reinterpret_cast<const char*>(&login_size), sizeof(login_size));
        file.write(reinterpret_cast<const char*>(nonce_login.data()), nonce_login.size());
        file.write(reinterpret_cast<const char*>(ciphertext_login.data()), ciphertext_login.size());

        uint32_t password_size = static_cast<uint32_t>(ciphertext_password.size());
        file.write(reinterpret_cast<const char*>(&password_size), sizeof(password_size));
        file.write(reinterpret_cast<const char*>(nonce_password.data()), nonce_password.size());
        file.write(reinterpret_cast<const char*>(ciphertext_password.data()), ciphertext_password.size());
    }

    static void load_credentials(const std::string& filename, std::string& login, std::string& password, const std::vector<unsigned char>& key) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for loading credentials");
        }

        uint32_t login_size;
        file.read(reinterpret_cast<char*>(&login_size), sizeof(login_size));

        std::vector<unsigned char> nonce_login(crypto_secretbox_NONCEBYTES);
        file.read(reinterpret_cast<char*>(nonce_login.data()), nonce_login.size());

        std::vector<unsigned char> ciphertext_login(login_size);
        file.read(reinterpret_cast<char*>(ciphertext_login.data()), login_size);

        decrypt(ciphertext_login, nonce_login, login, key);

        uint32_t password_size;
        file.read(reinterpret_cast<char*>(&password_size), sizeof(password_size));

        std::vector<unsigned char> nonce_password(crypto_secretbox_NONCEBYTES);
        file.read(reinterpret_cast<char*>(nonce_password.data()), nonce_password.size());

        std::vector<unsigned char> ciphertext_password(password_size);
        file.read(reinterpret_cast<char*>(ciphertext_password.data()), password_size);

        decrypt(ciphertext_password, nonce_password, password, key);
    }

};
