#pragma once
#include <string>
#include <vector>
#include <optional>

class User {
public:
    User(int id, const std::string& username, const std::string& passwordHash,
        const std::string& createdAt, std::optional<std::string> lastLogin = std::nullopt)
        : id(id), username(username), passwordHash(passwordHash),
        createdAt(createdAt), lastLogin(lastLogin) {}

    int getId() const { return id; }
    const std::string& getUsername() const { return username; }
    const std::string& getPasswordHash() const { return passwordHash; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::optional<std::string>& getLastLogin() const { return lastLogin; }

private:
    int id;
    std::string username;
    std::string passwordHash;
    std::string createdAt;
    std::optional<std::string> lastLogin; 
};

class Chat {
public:
    Chat(int id, const std::string& chatName, const std::string& createdAt,
        std::optional<std::string> lastActivity = std::nullopt)
        : id(id), chatName(chatName), createdAt(createdAt), lastActivity(lastActivity) {}

    int getId() const { return id; }
    const std::string& getChatName() const { return chatName; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::optional<std::string>& getLastActivity() const { return lastActivity; }

private:
    int id;
    std::string chatName;
    std::string createdAt;
    std::optional<std::string> lastActivity; 
};

class ChatMember {
public:
    ChatMember(int chatId, int userId, const std::string& joinedAt)
        : chatId(chatId), userId(userId), joinedAt(joinedAt) {}

    int getChatId() const { return chatId; }
    int getUserId() const { return userId; }
    const std::string& getJoinedAt() const { return joinedAt; }

private:
    int chatId;
    int userId;
    std::string joinedAt;
};

class Message {
public:
    Message(int id, int chatId, int userId, const std::string& messageText, const std::string& createdAt)
        : id(id), chatId(chatId), userId(userId), messageText(messageText), createdAt(createdAt) {}

    int getId() const { return id; }
    int getChatId() const { return chatId; }
    int getUserId() const { return userId; }
    const std::string& getMessageText() const { return messageText; }
    const std::string& getCreatedAt() const { return createdAt; }

private:
    int id;
    int chatId;
    int userId;
    std::string messageText;
    std::string createdAt;
};

class Media {
public:
    Media(int id, int messageId, const std::string& mediaType, const std::string& mediaPath, const std::string& createdAt)
        : id(id), messageId(messageId), mediaType(mediaType), mediaPath(mediaPath), createdAt(createdAt) {}

    int getId() const { return id; }
    int getMessageId() const { return messageId; }
    const std::string& getMediaType() const { return mediaType; }
    const std::string& getMediaPath() const { return mediaPath; }
    const std::string& getCreatedAt() const { return createdAt; }

private:
    int id;
    int messageId;
    std::string mediaType;
    std::string mediaPath;
    std::string createdAt;
};
