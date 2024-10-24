#pragma once
#include <vector>
#include <string>
#include <random>
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <argon2.h>
#include <sqlite3.h>
#include <spdlog/spdlog.h>

#include <data/SQLModels.hpp>
#include <data/PasswordManager.hpp>
class DatabaseManager {
public:
    enum class StatusCode {
        SUCCESS,
        USER_ALREADY_EXISTS,
        INSERT_ERROR,
        LOGIN_FAILED,
        USER_NOT_FOUND,
        _ERROR,
        Insufficient_Rights_ERROR,
        COUNT
    };

    DatabaseManager(const std::string& dbName = "data.db") {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            spdlog::error("Error opening database: {}", sqlite3_errmsg(db));
            sqlite3_close(db);
            db = nullptr;
            return;
        }
        createTables();
    }

    ~DatabaseManager() {
        if (db) {
            sqlite3_close(db);
        }
    }

    void createTables();

    void executeSQL(const char* sql);

    StatusCode registerUser(const std::string& username, const std::string& password);
    StatusCode loginUser(const std::string& username, const std::string& password, int& id);

    StatusCode createChat(const std::string& chatName, int& chatId, const std::string& chatType = "private");
    StatusCode addChatMember(int chatId, int userId, const std::string& role = "member", int adderId = 0);
    StatusCode removeChatMember(int chatId, int userId);

    StatusCode insertMessage(int chatId, int userId, const std::string& messageText);
    StatusCode insertMedia(int messageId, const std::string& mediaType, const std::string& mediaPath);

    std::vector<Media> getMediaByMessageId(int messageId);
    std::vector<Message> getMessages(int chatId);
    std::optional<User> GetUser(int userId);
    std::optional<Chat> GetChat(int chatId);

    bool isUserInChat(int chatId, int userId);

    StatusCode deleteUser(int userId);
    StatusCode updateUserProfile(int userId, const std::string& fieldName, const std::string& newValue);
    std::vector<Chat> getUserChats(int userId);

private:
    sqlite3* db = nullptr;
   

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};