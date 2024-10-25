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
        if (crypto_secretbox_easy(ciphertext.data(), (const unsigned char*)plaintext.c_str(), plaintext.size(), nonce.data(), key.data()) != 0) {
            throw std::runtime_error("Encryption failed");
        }
    }

    static void decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& nonce, std::string& plaintext, const std::vector<unsigned char>& key) {
        plaintext.resize(ciphertext.size() - crypto_secretbox_MACBYTES);
        if (crypto_secretbox_open_easy((unsigned char*)plaintext.data(), ciphertext.data(), ciphertext.size(), nonce.data(), key.data()) != 0) {
            throw std::runtime_error("Decryption failed");
        }
    }

    static void save_to_file(const std::string& filename, const std::vector<unsigned char>& nonce, const std::vector<unsigned char>& ciphertext) {
        std::ofstream file(filename, std::ios::binary);
        file.write((const char*)nonce.data(), nonce.size());
        file.write((const char*)ciphertext.data(), ciphertext.size());
    }

    static void load_from_file(const std::string& filename, std::vector<unsigned char>& nonce, std::vector<unsigned char>& ciphertext) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file");
        }
        nonce.resize(crypto_secretbox_NONCEBYTES);
        file.read((char*)nonce.data(), nonce.size());

        std::streampos current_pos = file.tellg();

        file.seekg(0, std::ios::end);
        std::streampos end_pos = file.tellg();

        std::streamoff size = static_cast<std::streamoff>(end_pos - current_pos);
        ciphertext.resize(size);

        file.seekg(current_pos);
        file.read((char*)ciphertext.data(), size);
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

        file.write((const char*)nonce_login.data(), nonce_login.size());
        file.write((const char*)ciphertext_login.data(), ciphertext_login.size());

        file.write((const char*)nonce_password.data(), nonce_password.size());
        file.write((const char*)ciphertext_password.data(), ciphertext_password.size());
    }

    static void load_credentials(const std::string& filename, std::string& login, std::string& password, const std::vector<unsigned char>& key) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for loading credentials");
        }

        std::vector<unsigned char> nonce_login(crypto_secretbox_NONCEBYTES);
        std::vector<unsigned char> ciphertext_login;
        file.read((char*)nonce_login.data(), nonce_login.size());

        std::streampos current_pos = file.tellg();
        file.seekg(0, std::ios::end);
        std::streampos end_pos = file.tellg();
        std::streamoff size_login = static_cast<std::streamoff>(end_pos - current_pos - crypto_secretbox_NONCEBYTES);
        ciphertext_login.resize(size_login);
        file.seekg(current_pos);
        file.read((char*)ciphertext_login.data(), size_login);

        decrypt(ciphertext_login, nonce_login, login, key);

        std::vector<unsigned char> nonce_password(crypto_secretbox_NONCEBYTES);
        std::vector<unsigned char> ciphertext_password;
        file.read((char*)nonce_password.data(), nonce_password.size());

        current_pos = file.tellg();
        file.seekg(0, std::ios::end);
        end_pos = file.tellg();
        std::streamoff size_password = static_cast<std::streamoff>(end_pos - current_pos);
        ciphertext_password.resize(size_password);
        file.seekg(current_pos);
        file.read((char*)ciphertext_password.data(), size_password);

        decrypt(ciphertext_password, nonce_password, password, key);
    }

};
