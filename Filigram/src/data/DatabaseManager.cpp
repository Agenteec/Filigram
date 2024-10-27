#include <data/DatabaseManager.h>
using StatusCode = DatabaseManager::StatusCode;

void DatabaseManager::createTables() {
    const char* createUsersTable = R"(
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        last_login DATETIME,
        email TEXT,
        profile_picture TEXT,
        bio TEXT,
        status TEXT DEFAULT 'offline',
        first_name TEXT,
        last_name TEXT,
        date_of_birth TEXT
    );
)";


    const char* createChatsTable = R"(
        CREATE TABLE IF NOT EXISTS chats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            chat_name TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_activity DATETIME,
            chat_type TEXT DEFAULT 'private'
        );
    )";

    const char* createChatMembersTable = R"(
        CREATE TABLE IF NOT EXISTS chat_members (
            chat_id INTEGER,
            user_id INTEGER,
            joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            role TEXT DEFAULT 'member',
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
            status TEXT DEFAULT 'sent',
            reply_to_message_id INTEGER,
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
            size INTEGER,
            FOREIGN KEY (message_id) REFERENCES messages(id)
        );
    )";

    executeSQL(createUsersTable);
    executeSQL(createChatsTable);
    executeSQL(createChatMembersTable);
    executeSQL(createMessagesTable);
    executeSQL(createMediaTable);
}

void DatabaseManager::executeSQL(const char* sql)
{
    char* errorMessage = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        spdlog::error("SQL execution error: {}", errorMessage);
        sqlite3_free(errorMessage);
    }
}

StatusCode DatabaseManager::loginUser(const std::string& username, const std::string& password, int& id) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    std::string checkUserSQL = "SELECT id, password_hash FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, checkUserSQL.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);
        const void* storedHashBlob = sqlite3_column_blob(stmt, 1);
        int storedHashSize = sqlite3_column_bytes(stmt, 1);
        std::vector<uint8_t> storedHash(static_cast<const uint8_t*>(storedHashBlob),
            static_cast<const uint8_t*>(storedHashBlob) + storedHashSize);
        if (PasswordManager::checkPassword(password, storedHash)) {
            sqlite3_finalize(stmt);

            std::string updateLastLoginSQL = "UPDATE users SET last_login = ? WHERE id = ?";
            sqlite3_stmt* updateStmt;
            if (sqlite3_prepare_v2(db, updateLastLoginSQL.c_str(), -1, &updateStmt, nullptr) != SQLITE_OK) {
                spdlog::error("Failed to prepare update statement: {}", sqlite3_errmsg(db));
                id = -1;
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                return StatusCode::_ERROR;
            }

            std::string currentTime = getCurrentTime();
            sqlite3_bind_text(updateStmt, 1, currentTime.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(updateStmt, 2, userId);

            if (sqlite3_step(updateStmt) != SQLITE_DONE) {
                spdlog::error("Failed to update last login: {}", sqlite3_errmsg(db));
                sqlite3_finalize(updateStmt);
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                id = -1;
                return StatusCode::_ERROR;
            }

            sqlite3_finalize(updateStmt);
            sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
            id = userId;
            return StatusCode::SUCCESS;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    id = -1;
    return StatusCode::LOGIN_FAILED;
}

StatusCode DatabaseManager::registerUser(const std::string& username, const std::string& password, int& userId, const std::string& firstName, const std::string& profilePicture) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

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
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::USER_ALREADY_EXISTS;
    }
    std::vector<uint8_t> passwordHash = PasswordManager::hashPassword(password);
    std::string insertUserSQL = "INSERT INTO users (username, password_hash, profile_picture, ) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(db, insertUserSQL.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, passwordHash.data(), passwordHash.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, profilePicture.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, profilePicture.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::INSERT_ERROR;
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    userId = sqlite3_last_insert_rowid(db);
    return StatusCode::SUCCESS;
}

StatusCode DatabaseManager::createChat(const std::string& chatName, int& chatId,const std::string& chatType) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    std::string insertChatSQL = "INSERT INTO chats (chat_name, created_at, last_activity, chat_type) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insertChatSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    std::string currentTime = getCurrentTime();

    sqlite3_bind_text(stmt, 1, chatName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, nullptr, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, chatType.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        spdlog::error("Failed to create chat: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    chatId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return StatusCode::SUCCESS;
}

StatusCode DatabaseManager::addChatMember(int chatId, int userId,const std::string& role,  int adderId) {
    if (adderId!= -1 &&!isUserInChat(chatId, adderId)) {
        spdlog::error("User {} is not a member of chat {}", adderId, chatId);
        return StatusCode::Insufficient_Rights_ERROR;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    std::string insertMemberSQL = "INSERT INTO chat_members (chat_id, user_id, joined_at, role) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insertMemberSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    std::string currentTime = getCurrentTime();

    sqlite3_bind_int(stmt, 1, chatId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_text(stmt, 3, currentTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, role.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        spdlog::error("Failed to add chat member: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return StatusCode::SUCCESS;
}

StatusCode DatabaseManager::removeChatMember(int chatId, int userId) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    std::string deleteMemberSQL = "DELETE FROM chat_members WHERE chat_id = ? AND user_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, deleteMemberSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    sqlite3_bind_int(stmt, 1, chatId);
    sqlite3_bind_int(stmt, 2, userId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        spdlog::error("Failed to remove chat member: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return StatusCode::SUCCESS;
}

StatusCode DatabaseManager::insertMessage(int chatId, int userId, const std::string& messageText, int& messageId) {
    std::string sql = "INSERT INTO messages (chat_id, user_id, message_text, created_at) VALUES (?, ?, ?, datetime('now'));";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare insert statement: {}", sqlite3_errmsg(db));
        return StatusCode::INSERT_ERROR;
    }

    sqlite3_bind_int(stmt, 1, chatId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_text(stmt, 3, messageText.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        spdlog::error("Failed to insert message: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return StatusCode::INSERT_ERROR;
    }

    messageId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return StatusCode::SUCCESS;
}

StatusCode DatabaseManager::insertMedia(int messageId, const std::string& mediaType, const std::string& mediaPath) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    std::string insertMediaSQL = "INSERT INTO media (message_id, media_type, media_path) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, insertMediaSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    sqlite3_bind_int(stmt, 1, messageId);
    sqlite3_bind_text(stmt, 2, mediaType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, mediaPath.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        spdlog::error("Failed to insert media: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return StatusCode::_ERROR;
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return StatusCode::SUCCESS;
}

std::vector<Media> DatabaseManager::getMediaByMessageId(int messageId) {
    std::vector<Media> mediaList;
    std::string selectMediaSQL = "SELECT id, message_id, media_type, media_path, created_at FROM media WHERE message_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectMediaSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        return mediaList;
    }

    sqlite3_bind_int(stmt, 1, messageId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int msgId = sqlite3_column_int(stmt, 1);
        const char* mediaType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* mediaPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        mediaList.emplace_back(id, msgId, mediaType ? mediaType : "", mediaPath ? mediaPath : "", createdAt ? createdAt : "");
    }

    sqlite3_finalize(stmt);
    return mediaList;
}

std::vector<Message> DatabaseManager::getMessages(int chatId)
{
    std::vector<Message> messages;
    std::string selectMessagesSQL = "SELECT id, chat_id, user_id, message_text, created_at FROM messages WHERE chat_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, selectMessagesSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        return messages;
    }

    sqlite3_bind_int(stmt, 1, chatId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int chatId = sqlite3_column_int(stmt, 1);
        int userId = sqlite3_column_int(stmt, 2);
        const char* messageText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));


        messages.emplace_back(id, chatId, userId, messageText ? messageText : "", createdAt ? createdAt : "");
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::optional<User> DatabaseManager::GetUser(int userId)
{
    std::string getUserSQL = "SELECT id, username, created_at, email, profile_picture, bio, status, first_name, last_name, date_of_birth FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, getUserSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, userId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {

        int id = sqlite3_column_int(stmt, 0);
        std::string username = (sqlite3_column_type(stmt, 1) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string createdAt = (sqlite3_column_type(stmt, 2) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string email = (sqlite3_column_type(stmt, 3) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string profilePicture = (sqlite3_column_type(stmt, 4) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string bio = (sqlite3_column_type(stmt, 5) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string status = (sqlite3_column_type(stmt, 6) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string firstName = (sqlite3_column_type(stmt, 7) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        std::string lastName = (sqlite3_column_type(stmt, 8) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        std::string dateOfBirth = (sqlite3_column_type(stmt, 9) == SQLITE_NULL) ? "" : reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));


        User user(id, username, "", createdAt, std::nullopt, email, profilePicture, bio, firstName, lastName, dateOfBirth);

        sqlite3_finalize(stmt);
        return user;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<Chat> DatabaseManager::GetChat(int chatId)
{

    std::string getChatSQL = "SELECT id, chat_name, created_at, last_activity, chat_type FROM chats WHERE id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, getChatSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, chatId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string chatName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::optional<std::string> lastActivity;
        const char* lastActivityText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (lastActivityText) {
            lastActivity = lastActivityText;
        }
        std::string chatType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        Chat chat(id, chatName, createdAt, lastActivity, chatType);

        sqlite3_finalize(stmt);

        std::string getMembersSQL = "SELECT user_id, joined_at, role FROM chat_members WHERE chat_id = ?";
        sqlite3_stmt* memberStmt;

        if (sqlite3_prepare_v2(db, getMembersSQL.c_str(), -1, &memberStmt, nullptr) != SQLITE_OK) {
            spdlog::error("Failed to prepare member statement: {}", sqlite3_errmsg(db));
            return std::nullopt;
        }

        sqlite3_bind_int(memberStmt, 1, chatId);

        while (sqlite3_step(memberStmt) == SQLITE_ROW) {
            int userId = sqlite3_column_int(memberStmt, 0);
            std::string joinedAt = reinterpret_cast<const char*>(sqlite3_column_text(memberStmt, 1));
            std::string role = reinterpret_cast<const char*>(sqlite3_column_text(memberStmt, 2));

            auto member = std::make_shared<ChatMember>(chatId, userId, joinedAt, role);
            chat.addMember(member);
        }

        sqlite3_finalize(memberStmt);
        return chat;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<ChatMember> DatabaseManager::getChatMembers(int chatId) {
    std::vector<ChatMember> members;
    std::string sql = "SELECT user_id, role, joined_at FROM chat_members WHERE chat_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare get chat members statement: {}", sqlite3_errmsg(db));
        return members;
    }

    sqlite3_bind_int(stmt, 1, chatId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);
        std::string role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string joinedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        members.emplace_back(chatId, userId, joinedAt, role);
    }

    sqlite3_finalize(stmt);
    return members;
}

StatusCode DatabaseManager::deleteUser(int userId) {
    std::string deleteSQL = "DELETE FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, deleteSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare delete statement: {}", sqlite3_errmsg(db));
        return StatusCode::_ERROR;
    }

    sqlite3_bind_int(stmt, 1, userId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return StatusCode::_ERROR;
    }

    sqlite3_finalize(stmt);
    return StatusCode::SUCCESS;
}

StatusCode DatabaseManager::updateUserProfile(int userId, const std::string& fieldName, const std::string& newValue) {
    std::string updateSQL = "UPDATE users SET " + fieldName + " = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, updateSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare update statement: {}", sqlite3_errmsg(db));
        return StatusCode::_ERROR;
    }

    sqlite3_bind_text(stmt, 1, newValue.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return StatusCode::_ERROR;
    }

    sqlite3_finalize(stmt);
    return StatusCode::SUCCESS;
}

std::vector<Chat> DatabaseManager::getUserChats(int userId) {
    std::vector<Chat> chats;
    std::string selectSQL = R"(
        SELECT chats.id, chats.chat_name, chats.created_at, chats.chat_type
        FROM chats
        JOIN chat_members ON chats.id = chat_members.chat_id
        WHERE chat_members.user_id = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int chatId = sqlite3_column_int(stmt, 0);
            std::string chatName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            std::string chatType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

            chats.emplace_back(chatId, chatName, createdAt, std::nullopt, chatType);
        }
    }
    sqlite3_finalize(stmt);

    return chats;
}

std::string DatabaseManager::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool DatabaseManager::isUserInChat(int chatId, int userId) {
    std::string checkMemberSQL = "SELECT COUNT(*) FROM chat_members WHERE chat_id = ? AND user_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, checkMemberSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        spdlog::error("Failed to prepare statement: {}", sqlite3_errmsg(db));
        return false;
    }

    sqlite3_bind_int(stmt, 1, chatId);
    sqlite3_bind_int(stmt, 2, userId);

    bool isMember = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        isMember = (count > 0);
    }

    sqlite3_finalize(stmt);
    return isMember;
}