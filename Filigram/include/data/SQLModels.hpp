#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
class User;
class Chat;
class Message;

class Media : public std::enable_shared_from_this<Media> {
public:
    Media(int id, int messageId, const std::string& mediaType, const std::string& mediaPath,
        const std::string& metaPath, const std::string& createdAt, size_t size = 0)
        : id(id), messageId(messageId), mediaType(mediaType), mediaPath(mediaPath), metaPath(metaPath),
        createdAt(createdAt), size(size) {}
    Media(int id, int messageId, const std::string& mediaType, const std::string& mediaPath,
        const std::string& metaPath)
        : id(id), messageId(messageId), mediaType(mediaType), mediaPath(mediaPath), metaPath(metaPath),
        createdAt(""), size(0) {}
    Media() :id(-1) {}
    int getId() const { return id; }
    int getMessageId() const { return messageId; }
    const std::string& getMediaType()const { return mediaType; }
    const std::string& getMetaPath()const { return metaPath; }
    const std::string& getMediaPath()const { return mediaPath; }
    const std::string& getCreatedAt()const { return createdAt; }

    std::shared_ptr<Message> message{ nullptr };
    

    void setId(int id) { this->id = id; }

    nlohmann::json metaData;
    std::vector<char> data;
private:
    int id;
    int messageId;
    std::string mediaType;
    std::string mediaPath;
    std::string metaPath;
    std::string createdAt;

    size_t size;
};

class ChatMember : public std::enable_shared_from_this<ChatMember> {
public:
    ChatMember(int chatId, int userId, const std::string& joinedAt, const std::string& role = "member")
        : chatId(chatId), userId(userId), joinedAt(joinedAt), role(role) {}

    int getChatId() const { return chatId; }
    int getUserId() const { return userId; }
    const std::string& getJoinedAt() const { return joinedAt; }
    const std::string& getRole() const { return role; }
    void setRole(const std::string& newRole) { role = newRole; }

    std::shared_ptr<User> user{ nullptr };
    std::shared_ptr<Chat> chat{ nullptr };

private:
    int chatId;
    int userId;
    std::string joinedAt;
    std::string role; // "admin", "member"
};
class Message : public std::enable_shared_from_this<Message> {
public:
    Message(int id = 0, int chatId = 0, int userId = 0, const std::string& messageText = "", const std::string& createdAt = "",
        const std::string& status = "sent", std::optional<int> replyToMessageId = std::nullopt)
        : id(id), chatId(chatId), userId(userId), messageText(messageText),
        createdAt(createdAt), status(status), replyToMessageId(replyToMessageId) {}

    int getId() const { return id; }
    int getChatId() const { return chatId; }
    int getUserId() const { return userId; }
    const std::string& getMessageText() const { return messageText; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::string& getStatus() const { return status; }
    const std::optional<int>& getReplyToMessageId()const  { return replyToMessageId; };

    void addMedia(std::shared_ptr<Media> media) {
        _media[media->getId()] = media;
    }

    void removeMedia(int mediaId) {
        _media.erase(mediaId);
    }

    const std::unordered_map<int, std::shared_ptr<Media>>& getMedia() const { return _media; }

    std::shared_ptr<User> user;
    std::shared_ptr<Chat> chat;
    std::shared_ptr<Message> replyToMessage;

private:
    int id;
    int chatId;
    int userId;
    std::string messageText;
    std::string createdAt;
    std::string status; // "sent", "delivered", "read"
    std::optional<int> replyToMessageId;
    std::unordered_map<int, std::shared_ptr<Media>> _media;
};

class Chat : public std::enable_shared_from_this<Chat> {
public:
    Chat(int id, const std::string& chatName, const std::string& createdAt,
        std::optional<std::string> lastActivity = std::nullopt,
        const std::string& chatType = "private")
        : id(id), chatName(chatName), createdAt(createdAt),
        lastActivity(lastActivity), chatType(chatType) {}

    int getId() const { return id; }
    const std::string& getChatName() const { return chatName; }
    const std::string& getCreatedAt() const { return createdAt; }
    const std::optional<std::string>& getLastActivity() const { return lastActivity; }
    const std::string& getChatType() const { return chatType; }

    void addMember(std::shared_ptr<ChatMember> member) {
        members[member->getUserId()] = member;
    }

    void removeMember(int userId) {
        members.erase(userId);
    }

    void addMessage(std::shared_ptr<Message> message) {
        messages[message->getId()] = message;
    }

    void removeMessage(int messageId) {
        messages.erase(messageId);
    }

    const std::unordered_map<int, std::shared_ptr<ChatMember>>& getMembers() const { return members; }
    const std::map<int, std::shared_ptr<Message>>& getMessages() const { return messages; }

private:
    int id;
    std::string chatName;
    std::string createdAt;
    std::optional<std::string> lastActivity;
    std::string chatType; // "group", "private"
    std::unordered_map<int, std::shared_ptr<ChatMember>> members;
    std::map<int, std::shared_ptr<Message>> messages;
};

class User : public std::enable_shared_from_this<User> {
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

    void setUsername(const std::string& newUsername) { username = newUsername; }
    void setPasswordHash(const std::string& newPasswordHash) { passwordHash = newPasswordHash; }
    void setLastLogin(const std::optional<std::string>& newLastLogin) { lastLogin = newLastLogin; }
    void setEmail(const std::string& newEmail) { email = newEmail; }
    void setProfilePicture(const std::string& newProfilePicture) { profilePicture = newProfilePicture; }
    void setBio(const std::string& newBio) { bio = newBio; }
    void setStatus(const std::string& newStatus) { status = newStatus; }
    void setFirstName(const std::string& newFirstName) { firstName = newFirstName; }
    void setLastName(const std::string& newLastName) { lastName = newLastName; }
    void setDateOfBirth(const std::string& newDateOfBirth) { dateOfBirth = newDateOfBirth; }
    sf::Color nameColor;
    std::shared_ptr<sf::Texture> profileTexture;
    std::shared_ptr<sf::Texture> getProfilePictureTexture() {
        if (!profileTexture) {
            profileTexture = std::make_shared<sf::Texture>();

            if (profilePicture.empty() || profilePicture[0] == ':') {
                
                if (!profileTexture->loadFromFile("Assets/images/default.png")) {
                    spdlog::error("Error loading profile photo from file: Assets/images/default.png");
                    profileTexture = std::make_shared<sf::Texture>();
                }
            }
            else {
                if (!profileTexture->loadFromFile(profilePicture)) {
                    spdlog::error("Error loading profile photo from file: {}", profilePicture);
                    profileTexture = std::make_shared<sf::Texture>();
                }
            }
        }
        return profileTexture;
    }
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



class Notification : public std::enable_shared_from_this<Notification> {
public:
    Notification(int id, int userId, const std::string& message, const std::string& createdAt)
        : id(id), userId(userId), message(message), createdAt(createdAt), isRead(false) {}

    int getId() const { return id; }
    int getUserId() const { return userId; }
    const std::string& getMessage() const { return message; }
    const std::string& getCreatedAt() const { return createdAt; }
    bool isReadStatus() const { return isRead; }

    void markAsRead() { isRead = true; }

    std::shared_ptr<User> user{ nullptr };
private:
    int id;
    int userId;
    std::string message;
    std::string createdAt;
    bool isRead;
};

class Reaction : public std::enable_shared_from_this<Reaction> {
public:
    Reaction(int id, int messageId, int userId, const std::string& reactionType, const std::string& createdAt)
        : id(id), messageId(messageId), userId(userId), reactionType(reactionType), createdAt(createdAt) {}

    int getId() const { return id; }
    int getMessageId() const { return messageId; }
    int getUserId() const { return userId; }
    const std::string& getReactionType() const { return reactionType; }
    const std::string& getCreatedAt() const { return createdAt; }
    std::shared_ptr<User> user{ nullptr };
    std::shared_ptr<Message> message{ nullptr };
private:
    int id;
    int messageId;
    int userId;
    std::string reactionType; // "like", "love", "laugh"
    std::string createdAt;
};