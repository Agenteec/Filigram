#pragma once
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <spdlog/spdlog.h>

class DatabaseManager {
public:
    enum class StatusCode {
        SUCCESS,
        USER_ALREADY_EXISTS,
        INSERT_ERROR,
        LOGIN_FAILED,
        USER_NOT_FOUND,
        _ERROR,
        //--\\
        \\--//
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

    void createTables() {
        const char* createUsersTable = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                last_login DATETIME
            );
        )";

        const char* createChatsTable = R"(
            CREATE TABLE IF NOT EXISTS chats (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chat_name TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                last_activity DATETIME
            );
        )";

        const char* createChatMembersTable = R"(
            CREATE TABLE IF NOT EXISTS chat_members (
                chat_id INTEGER,
                user_id INTEGER,
                joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY (chat_id, user_id),
                FOREIGN KEY (chat_id) REFERENCES chats(id),
                FOREIGN KEY (user_id) REFERENCES users(id)
            );
        )";

        const char* createMessagesTable = R"(
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                chat_id INTEGER,
                user_id INTEGER,
                message_text TEXT NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (chat_id) REFERENCES chats(id),
                FOREIGN KEY (user_id) REFERENCES users(id)
            );
        )";

        const char* createMediaTable = R"(
            CREATE TABLE IF NOT EXISTS media (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                message_id INTEGER,
                media_type TEXT NOT NULL,
                media_path TEXT NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (message_id) REFERENCES messages(id)
            );
        )";

        executeSQL(createUsersTable);
        executeSQL(createChatsTable);
        executeSQL(createChatMembersTable);
        executeSQL(createMessagesTable);
        executeSQL(createMediaTable);
    }

    void executeSQL(const char* sql) {
        char* errorMessage = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            spdlog::error("SQL execution error: {}", errorMessage);
            sqlite3_free(errorMessage);
        }
    }
    StatusCode registerUser(const std::string& username, const std::string& password) {
        std::string checkUserSQL = "SELECT COUNT(*) FROM users WHERE username = ?";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, checkUserSQL.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        if (count > 0) {
            return StatusCode::USER_ALREADY_EXISTS;
        }

        // Хэширование
        std::string passwordHash = password; // Заменить

        std::string insertUserSQL = "INSERT INTO users (username, password_hash) VALUES (?, ?)";
        sqlite3_prepare_v2(db, insertUserSQL.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return StatusCode::INSERT_ERROR;
        }
        sqlite3_finalize(stmt);
        return StatusCode::SUCCESS;
    }

    StatusCode loginUser(const std::string& username, const std::string& password) {
        std::string checkUserSQL = "SELECT password_hash FROM users WHERE username = ?";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, checkUserSQL.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            // Хэширование
            if (storedHash == password) { // Заменить
                sqlite3_finalize(stmt);
                return StatusCode::SUCCESS;
            }
        }
        sqlite3_finalize(stmt);
        return StatusCode::LOGIN_FAILED;
    }
private:
    sqlite3* db = nullptr;
};
