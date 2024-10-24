#include <iostream>
#include <string>
#include <argon2.h>
#include <vector>
#include <random>

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
};
