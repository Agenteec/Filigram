#pragma once
#include <string>
#include <vector>
#include <optional>

class User {
public:
    User(int id, const std::string& username, const std::string& passwordHash,
        const std::string& createdAt, std::optional<std::string> lastLogin = std::nullopt,
        const std::string& email = "", const std::string& profilePicture = "",
        const std::string& bio = "", const std::string& firstName = "",
        const std::string& lastName = "", const std::string& dateOfBirth = "")
        : id(id), username(username), passwordHash(passwordHash),
        createdAt(createdAt), lastLogin(lastLogin), email(email),
        profilePicture(profilePicture), bio(bio), status("offline"),
        firstName(firstName), lastName(lastName), dateOfBirth(dateOfBirth) {}

    User() : id(-1) {}


    // Getters
    int getId() const { return id; }
    const std::string& getUsername() const { return username; }
    const std::string& getPasswordHash() const { return passwordHash; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::optional<std::string>& getLastLogin() const { return lastLogin; }
    const std::string& getEmail() const { return email; }
    const std::string& getProfilePicture() const { return profilePicture; }
    const std::string& getBio() const { return bio; }
    const std::string& getStatus() const { return status; }
    const std::string& getFirstName() const { return firstName; }
    const std::string& getLastName() const { return lastName; }
    const std::string& getDateOfBirth() const { return dateOfBirth; }

    // Setters
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void changePassword(const std::string& newPasswordHash) { passwordHash = newPasswordHash; }
    void updateEmail(const std::string& newEmail) { email = newEmail; }
    void updateProfilePicture(const std::string& newProfilePicture) { profilePicture = newProfilePicture; }
    void updateBio(const std::string& newBio) { bio = newBio; }
    void updateFirstName(const std::string& newFirstName) { firstName = newFirstName; }
    void updateLastName(const std::string& newLastName) { lastName = newLastName; }
    void updateDateOfBirth(const std::string& newDateOfBirth) { dateOfBirth = newDateOfBirth; }

private:
    int id;
    std::string username;
    std::string passwordHash;
    std::string createdAt;
    std::optional<std::string> lastLogin;
    std::string email;
    std::string profilePicture;
    std::string bio;
    std::string status; // "online", "offline", "do not disturb"
    std::string firstName;
    std::string lastName;
    std::string dateOfBirth;
};

class ChatMember {
public:
    ChatMember(int chatId, int userId, const std::string& joinedAt, const std::string& role = "member")
        : chatId(chatId), userId(userId), joinedAt(joinedAt), role(role) {}

    int getChatId() const { return chatId; }
    int getUserId() const { return userId; }
    const std::string& getJoinedAt() const { return joinedAt; }
    const std::string& getRole() const { return role; }

    void setRole(const std::string& newRole) { role = newRole; }

private:
    int chatId;
    int userId;
    std::string joinedAt;
    std::string role; // "admin", "member"
};

class Chat {
public:
    Chat(int id, const std::string& chatName, const std::string& createdAt,
        std::optional<std::string> lastActivity = std::nullopt,
        const std::string& chatType = "private",
        const std::vector<ChatMember>& members = {})
        : id(id), chatName(chatName), createdAt(createdAt),
        lastActivity(lastActivity), chatType(chatType), members(members) {}

    Chat() :id(-1){}

    int getId() const { return id; }
    const std::string& getChatName() const { return chatName; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::optional<std::string>& getLastActivity() const { return lastActivity; }
    const std::string& getChatType() const { return chatType; }


    void addMember(const ChatMember& member) {
        members.push_back(member);
    }

    void removeMember(int userId) {
        members.erase(std::remove_if(members.begin(), members.end(),
            [userId](const ChatMember& member) { return member.getUserId() == userId; }),
            members.end());
    }

    const std::vector<ChatMember>& getMembers() const { return members; }

private:
    int id;
    std::string chatName;
    std::string createdAt;
    std::optional<std::string> lastActivity;
    std::string chatType; // "group", "private"
    std::vector<ChatMember> members; // Список участников
};


class Message {
public:
    Message(int id, int chatId, int userId, const std::string& messageText, const std::string& createdAt,
        const std::string& status = "sent", std::optional<int> replyToMessageId = std::nullopt)
        : id(id), chatId(chatId), userId(userId), messageText(messageText),
        createdAt(createdAt), status(status), replyToMessageId(replyToMessageId) {}

    int getId() const { return id; }
    int getChatId() const { return chatId; }
    int getUserId() const { return userId; }
    const std::string& getMessageText() const { return messageText; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::string& getStatus() const { return status; }
    const std::optional<int>& getReplyToMessageId() const { return replyToMessageId; }

    void setStatus(const std::string& newStatus) { status = newStatus; }
    void editMessage(const std::string& newText) { messageText = newText; }

private:
    int id;
    int chatId;
    int userId;
    std::string messageText;
    std::string createdAt;
    std::string status; // "sent", "delivered", "read"
    std::optional<int> replyToMessageId;
};

class Media {
public:
    Media(int id, int messageId, const std::string& mediaType, const std::string& mediaPath,
        const std::string& createdAt, size_t size = 0)
        : id(id), messageId(messageId), mediaType(mediaType), mediaPath(mediaPath),
        createdAt(createdAt), size(size) {}

    int getId() const { return id; }
    int getMessageId() const { return messageId; }
    const std::string& getMediaType() const { return mediaType; }
    const std::string& getMediaPath() const { return mediaPath; }
    const std::string& getCreatedAt() const { return createdAt; }
    size_t getSize() const { return size; }

private:
    int id;
    int messageId;
    std::string mediaType; // "image", "video", "audio", etc.
    std::string mediaPath;
    std::string createdAt;
    size_t size; // Bytes
};

class Notification {
public:
    Notification(int id, int userId, const std::string& message, const std::string& createdAt)
        : id(id), userId(userId), message(message), createdAt(createdAt), isRead(false) {}

    int getId() const { return id; }
    int getUserId() const { return userId; }
    const std::string& getMessage() const { return message; }
    const std::string& getCreatedAt() const { return createdAt; }
    bool isReadStatus() const { return isRead; }

    void markAsRead() { isRead = true; }

private:
    int id;
    int userId;
    std::string message;
    std::string createdAt;
    bool isRead;
};

class Reaction {
public:
    Reaction(int id, int messageId, int userId, const std::string& reactionType, const std::string& createdAt)
        : id(id), messageId(messageId), userId(userId), reactionType(reactionType), createdAt(createdAt) {}

    int getId() const { return id; }
    int getMessageId() const { return messageId; }
    int getUserId() const { return userId; }
    const std::string& getReactionType() const { return reactionType; }
    const std::string& getCreatedAt() const { return createdAt; }

private:
    int id;
    int messageId;
    int userId;
    std::string reactionType; // "like", "love", "laugh"
    std::string createdAt;
};