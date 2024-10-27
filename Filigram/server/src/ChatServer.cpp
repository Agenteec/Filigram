#include <ChatServer.h>


ChatServer::ChatServer(unsigned short port) : dbManager("data.db") {
    if (listener.listen(port) != sf::Socket::Done) {
        spdlog::error("Error starting server on port {}", port);
    }
    else {
        spdlog::info("Server started on port {}", port);
    }
}

void ChatServer::run() {
    
    startPingThread();

    while (true) {
        auto client = std::make_shared<sf::TcpSocket>();
        if (listener.accept(*client) == sf::Socket::Done) {
            spdlog::info("New client connected: {}", client->getRemoteAddress().toString());
            std::thread(&ChatServer::handleClient, this, client).detach();
        }
    }
}

void ChatServer::handleClient(std::shared_ptr<sf::TcpSocket> client) {
    std::optional<User> currentUser = std::nullopt;
    sf::Packet packet;

    while (true) {
        if (client->receive(packet) != sf::Socket::Done) {
            spdlog::info("Client disconnected: {}", client->getRemoteAddress().toString());
            break;
        }
        json response;

        std::string data;
        packet >> data;
        try 
        {
            json request = json::parse(data);

            response["action"] = request["action"];
            if (request["action"] == "send_message") {
            int chatId = request["chat_id"];
            std::string messageText = request["message"];

            
            if (!currentUser || !dbManager.isUserInChat(chatId, currentUser->getId())) {
                response["status"] = "error";
                response["message"] = "Access denied or user not in chat.";
            }
            else {
                int messageId;
                if (dbManager.insertMessage(chatId, currentUser->getId(), messageText, messageId) == DatabaseManager::StatusCode::SUCCESS) {
                    response["status"] = "success";
                    response["message"] = "Message sent.";
                    response["message_id"] = messageId;
                    Message message(int id, int chatId, int userId, const std::string & messageText, const std::string & createdAt,
                        const std::string & status = "sent");
                    json messageJson;
                    messageJson["action"] = "new_message";
                    messageJson["chat_id"] = chatId;
                    messageJson["sender_id"] = currentUser->getId();
                    messageJson["message_id"] = messageId;
                    messageJson["message_text"] = messageText;
                    messageJson["timestamp"] = DatabaseManager::getCurrentTime();
                    broadcastMessageToChat(chatId, messageJson);
                }
                else {
                    response["status"] = "error";
                    response["message"] = "Failed to save message.";
                }
            }
        }
            else if (request["action"] == "ping") {
                response["status"] = "pong";
                response["action"] = "ping";
            }
            else if (request["action"] == "get_chat_members") {
                if (!currentUser.has_value()) {
                    response["status"] = "error";
                    response["message"] = "User not logged in.";
                }
                else {
                    int chatId = request["chat_id"];
                    auto chatMembers = dbManager.getChatMembers(chatId);

                    json membersArray = json::array();
                    for (const auto& member : chatMembers) {
                        json memberData;
                        memberData["user_id"] = member.getUserId();
                        memberData["joined_at"] = member.getJoinedAt();
                        memberData["role"] = member.getRole();
                        membersArray.push_back(memberData);
                    }

                    response["status"] = "success";
                    response["members"] = membersArray;
                    response["message"] = "Chat members retrieved successfully.";
                }
            }
            else if (request["action"] == "login") {
                std::string username = request["username"];
                std::string password = request["password"];
                int userId = -1;

                if (dbManager.loginUser(username, password, userId) == DatabaseManager::StatusCode::SUCCESS) {
                    response["status"] = "success";
                    response["message"] = "Login successful.";
                    currentUser = dbManager.GetUser(userId);
                    response["user_id"] = currentUser->getId();
                    clientSockets[userId] = client;
                }
                else {
                    response["status"] = "error";
                    response["message"] = "Login failed.";
                }
            }
            else if (request["action"] == "get_user_chats") {
                if (!currentUser) {
                    response["status"] = "error";
                    response["message"] = "User is not authenticated.";
                }
                else {
                    auto chats = dbManager.getUserChats(currentUser->getId());

                    response["status"] = "success";
                    response["message"] = "Chats retrieved successfully.";

                    json chatList = json::array();
                    std::set<int> userIds;

                    for (const auto& chat : chats) {
                        json chatJson;
                        chatJson["id"] = chat.getId();
                        chatJson["name"] = chat.getChatName();
                        chatJson["created_at"] = chat.getCreatedAt();
                        chatJson["last_activity"] = chat.getLastActivity().value_or("");
                        chatJson["chat_type"] = chat.getChatType();

                        auto members = dbManager.getChatMembers(chat.getId());
                        json membersJson = json::array();
                        for (const auto& member : members) {
                            json memberJson;
                            memberJson["user_id"] = member.getUserId();
                            memberJson["joined_at"] = member.getJoinedAt();
                            memberJson["role"] = member.getRole();

                            auto user = dbManager.GetUser(member.getUserId());
                            if (user) {
                                memberJson["username"] = user->getUsername();
                                memberJson["email"] = user->getEmail();
                                memberJson["profile_picture"] = user->getProfilePicture();
                                userIds.insert(user->getId());
                            }
                            membersJson.push_back(memberJson);
                        }
                        chatJson["members"] = membersJson;

                        auto messages = dbManager.getMessages(chat.getId());
                        json messagesJson = json::array();
                        for (const auto& message : messages) {
                            json messageJson;
                            messageJson["message_id"] = message.getId();
                            messageJson["chat_id"] = message.getChatId();
                            messageJson["user_id"] = message.getUserId();
                            messageJson["message_text"] = message.getMessageText();
                            messageJson["created_at"] = message.getCreatedAt();
                            messageJson["status"] = message.getStatus();

                            auto sender = dbManager.GetUser(message.getUserId());
                            if (sender) {
                                messageJson["username"] = sender->getUsername();
                                messageJson["profile_picture"] = sender->getProfilePicture();
                                userIds.insert(sender->getId());
                            }
                            messagesJson.push_back(messageJson);
                        }
                        chatJson["messages"] = messagesJson;
                        chatList.push_back(chatJson);
                    }
                    response["chats"] = chatList;

                    json usersJson = json::array();
                    for (int userId : userIds) {
                        auto user = dbManager.GetUser(userId);
                        if (user) {
                            json userJson;
                            userJson["id"] = user->getId();
                            userJson["username"] = user->getUsername();
                            userJson["created_at"] = user->getCreatedAt();
                            userJson["last_login"] = user->getLastLogin().value_or("");
                            userJson["email"] = user->getEmail();
                            userJson["profile_picture"] = user->getProfilePicture();
                            userJson["bio"] = user->getBio();
                            userJson["first_name"] = user->getFirstName();
                            userJson["last_name"] = user->getLastName();
                            userJson["date_of_birth"] = user->getDateOfBirth();
                            usersJson.push_back(userJson);
                        }
                    }
                    response["users"] = usersJson;
                }
            }
            else if (request["action"] == "get_chat_messages") {
                int chatId = request["chat_id"];

                
                if (!currentUser || !dbManager.isUserInChat(chatId, currentUser->getId())) {
                    response["status"] = "error";
                    response["message"] = "Access denied or chat not found.";
                }
                else {
                    auto messages = dbManager.getMessages(chatId);

                    response["status"] = "success";
                    response["message"] = "Messages retrieved successfully.";

                    json messageList = json::array();
                    for (const auto& message : messages) {
                        json messageJson;
                        messageJson["id"] = message.getId();
                        messageJson["user_id"] = message.getUserId();
                        messageJson["text"] = message.getMessageText();
                        messageJson["created_at"] = message.getCreatedAt();
                        messageJson["status"] = message.getStatus();
                        messageJson["reply_to"] = message.getReplyToMessageId().value_or(-1);
                        messageList.push_back(messageJson);
                    }
                    response["messages"] = messageList;
                }
            }
            else if (request["action"] == "get_user_info")
            {
                const auto& user = dbManager.GetUser(request["user_id"]);
                if (!currentUser) {
                    response["status"] = "error";
                    response["message"] = "User is not authenticated.";
                }
                else if (user == std::nullopt)
                {
                    response["status"] = "error";
                    response["message"] = "User is null.";
                }
                else
                {
                    response["user_bio"] = user->getBio();
                    response["user_created_at"] = user->getCreatedAt();
                    response["user_date_of_birth"] = user->getDateOfBirth();
                    response["user_email"] = user->getEmail();
                    response["user_first_name"] = user->getFirstName();
                    response["user_last_name"] = user->getLastName();
                    response["user_status"] = user->getStatus();
                    response["user_id"] = user->getId();

                    response["status"] = "success";
                    response["message"] = "User information received.";
                }

            }
            else if (request["action"] == "update_user_info")
            {
                
                    if (request["user_id"] != currentUser->getId())
                    {
                        response["status"] = "error";
                        response["message"] = "Permission denied.";
                    }
                    else
                    {
                        if (!request["username"].empty())
                            dbManager.updateUserProfile(request["user_id"],"username", request["username"]);
                        if (!request["user_email"].empty())
                            dbManager.updateUserProfile(request["user_id"], "email", request["user_email"]);
                        if (!request["user_profile_picture_data"].empty())
                            dbManager.updateUserProfile(request["user_id"], "profile_picture", request["user_profile_picture"]);
                        if (!request["user_bio"].empty())
                            dbManager.updateUserProfile(request["user_id"], "bio", request["user_bio"]);
                        if (!request["user_status"].empty())
                            dbManager.updateUserProfile(request["user_id"], "status", request["user_status"]);
                        if (!request["user_first_name"].empty())
                            dbManager.updateUserProfile(request["user_id"], "first_name", request["user_first_name"]);
                        if (!request["user_last_name"].empty())
                            dbManager.updateUserProfile(request["user_id"], "last_name", request["user_last_name"]);
                        if (!request["user_date_of_birth"].empty())
                            dbManager.updateUserProfile(request["user_id"], "date_of_birth", request["user_date_of_birth"]);

                        response["status"] = "success";
                        response["message"] = "User info updated.";
                        
                    }
            }
            else if (request["action"] == "register") {
                std::string username = request["username"];
                std::string password = request["password"];
                int id;
                auto status = dbManager.registerUser(username, password, id);

                if (status == DatabaseManager::StatusCode::SUCCESS) {

                    dbManager.addChatMember(1, id);
                    auto user = dbManager.GetUser(id);
                    if (user) {
                        json userJson; 
                        userJson["action"] = "new_chat_member";
                        userJson["id"] = user->getId();
                        userJson["username"] = user->getUsername();
                        userJson["created_at"] = user->getCreatedAt();
                        userJson["last_login"] = user->getLastLogin().value_or("");
                        userJson["email"] = user->getEmail();
                        userJson["profile_picture"] = user->getProfilePicture();
                        userJson["bio"] = user->getBio();
                        userJson["first_name"] = user->getFirstName();
                        userJson["last_name"] = user->getLastName();
                        userJson["date_of_birth"] = user->getDateOfBirth();
                        broadcastMessageToChat(1,userJson);
                    }

                    response["status"] = "success";
                    response["message"] = "Registration successful.";
                }
                else if (status == DatabaseManager::StatusCode::USER_ALREADY_EXISTS) {
                    response["status"] = "error";
                    response["message"] = "User already exists.";
                }
                else {
                    response["status"] = "error";
                    response["message"] = "Registration failed.";
                }
            }
            else {
                response["status"] = "error";
                response["message"] = "Unknown action.";
            }

        }
        catch (const json::parse_error& e) {
            response["action"] = "ex";
            response["status"] = "error";
            response["message"] = "Invalid JSON format.";
        }
        sf::Packet responsePacket;
        responsePacket << response.dump();
        client->send(responsePacket);
        
    }
    if (currentUser!=std::nullopt)
    {
        clientSockets.erase(currentUser->getId());
    }
}
void ChatServer::broadcastMessageToChat(int chatId, const json& messageJson) {
    auto chatMembers = dbManager.getChatMembers(chatId);

   

    std::string messageData = messageJson.dump();
    sf::Packet packet;
    packet << messageData;

    for (const auto& member : chatMembers) {
        int memberId = member.getUserId();

        if (clientSockets.find(memberId) != clientSockets.end()) {
            auto client = clientSockets[memberId];
            if (client->send(packet) != sf::Socket::Done) {
                spdlog::error("Failed to send message to user {} in chat {}", memberId, chatId);
            }
            else {
                spdlog::info("Message sent to user {} in chat {}", memberId, chatId);
            }
        }
    }
}

void ChatServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex);
    for (const auto& [userId, client] : clientSockets) {
        sf::Packet packet;
        packet << message;
        if (client->send(packet) != sf::Socket::Done) {
            spdlog::error("Failed to send message to client: {}", client->getRemoteAddress().toString());
        }
    }
    cv.notify_all();
}

void ChatServer::startPingThread() {
    std::thread([this]() {
        while (true) {
            
            std::this_thread::sleep_for(std::chrono::seconds(10));

            json pingRequest;
            pingRequest["action"] = "ping";
            pingRequest["status"] = "ping";
            std::string pingData = pingRequest.dump();

            broadcastMessage(pingData);
        }
        }).detach();
}