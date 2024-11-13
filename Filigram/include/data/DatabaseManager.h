#pragma once
#include <vector>
#include <string>
#include <random>
#include <stdexcept>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <memory>

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
        if (!GetChat(1))
        {
            int id;
            createChat("Global", id,"group");
        }
    }

    ~DatabaseManager() {
        if (db) {
            sqlite3_close(db);
        }
    }

    void createTables();

    void executeSQL(const char* sql);

    StatusCode registerUser(const std::string& username, const std::string& password, int& id, const std::string& firstName = "DefaultUsername", const std::string& profilePicture = ":profile_photo_default");
    StatusCode loginUser(const std::string& username, const std::string& password, int& id);

    StatusCode createChat(const std::string& chatName, int& chatId, const std::string& chatType = "private");
    StatusCode addChatMember(int chatId, int userId, const std::string& role = "member", int adderId = -1);
    StatusCode removeChatMember(int chatId, int userId);

    StatusCode insertMessage(int chatId, int userId, const std::string& messageText, int& messageId);
    StatusCode insertMedia(int messageId, const std::string& mediaType, const std::string& mediaPath, const std::string& metaPath, int& mediaId);
    StatusCode insertMedia(Media& media);

    std::vector<Media> getMediaByMessageId(int messageId);
    Media getMediaById(int mediaId);
    std::vector<Message> getMessages(int chatId);

    Message getMessageById(int messageId);

    std::optional<User> GetUser(int userId);
    std::optional<Chat> GetChat(int chatId);
    std::vector<ChatMember> getChatMembers(int chatId);
    bool isUserInChat(int chatId, int userId);

    StatusCode deleteUser(int userId);
    StatusCode updateUserProfile(int userId, const std::string& fieldName, const std::string& newValue);
    std::vector<Chat> getUserChats(int userId);

    static std::string getCurrentTime();
private:
    sqlite3* db = nullptr;


};