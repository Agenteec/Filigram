#include <ChatServer.h>
std::vector<char> ChatServer::readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Error reading file: " + filePath);
    }

    return buffer;
}


ChatServer::ChatServer(unsigned short port) : dbManager("data.db") {
    if (listener.listen(port) != sf::Socket::Status::Done) {
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
        if (listener.accept(*client) == sf::Socket::Status::Done) {
            spdlog::info("New client connected: {}", client->getRemoteAddress()->toString());
            std::thread(&ChatServer::handleClient, this, client).detach();
        }
    }
}

void ChatServer::handleClient(std::shared_ptr<sf::TcpSocket> client) {
    std::optional<User> currentUser = std::nullopt;
    sf::Packet packet;

    while (true) {
        if (client->receive(packet) != sf::Socket::Status::Done) {
            spdlog::info("Client disconnected: {}", client->getRemoteAddress()->toString());
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
            std::string messageType = request["message_type"];
            
            if (!currentUser || !dbManager.isUserInChat(chatId, currentUser->getId())) {
                response["status"] = "error";
                response["message"] = "Access denied or user not in chat.";
            }
            else {
                int messageId;
                if (dbManager.insertMessage(chatId, currentUser->getId(), messageText, messageId) == DatabaseManager::StatusCode::SUCCESS) {
                    json messageJson;
                    messageJson["action"] = "new_message";
                    messageJson["chat_id"] = chatId;
                    messageJson["timestamp"] = DatabaseManager::getCurrentTime();
                    messageJson["sender_id"] = currentUser->getId();
                    messageJson["message_id"] = messageId;
                    messageJson["message_text"] = messageText;
                    if (messageType == "plot") {
                        json plotData = request["plot_data"];
                        auto media = generatePlot(plotData, messageId);

                        json mediaJson = json::array();
                        
                        json med;
                        med["media_type"] = "plot";
                        med["media_path"] = media->getMediaPath();
                        med["created_at"] =  media->getCreatedAt();
                        med["meta_path"] = media->getMetaPath();
                        med["media_id"]  = media->getId();
                        mediaJson.push_back(med);
                        messageJson["media"] = mediaJson;
                    }
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
            else if (request["action"] == "get_media_data")
            {
                if (!currentUser.has_value()) {
                    response["status"] = "error";
                    response["message"] = "User not logged in.";
                }
                else
                {
                    json mediaIdsJson = request["media_ids"];
                    auto chats = dbManager.getUserChats(currentUser->getId());//-----------------------------------------------------------------------------------------------
                    json mediaJson = json::array();
                    for (int id : request["media_ids"])
                    {
                        Media media = dbManager.getMediaById(id);
                        auto message = dbManager.getMessageById(media.getMessageId());
                        bool isMember = false;
                        for (const auto chat: chats)
                        {
                            
                            if (chat.getId() == message.getChatId())
                            {
                                isMember = true;
                                break;
                            }
                        }
                        if (isMember)
                        {
                            

                            json med;
                            med["media_type"] = "plot";
                            med["media_path"] = media.getMediaPath();
                            med["created_at"] = media.getCreatedAt();
                            med["meta_path"] = media.getMetaPath();
                            med["media_id"] = media.getId();
                            med["message_id"] = media.getMessageId();
                            mediaJson.push_back(med);
                           
                        }
                    }
                    response["media"] = mediaJson;
                    response["status"] = "success";
                    response["message"] = "Media sent successfully.";
                }

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
                            auto media = dbManager.getMediaByMessageId(message.getId());
                            json mediaJson = json::array();
                            for (auto& med: media)
                            {
                                json medJson;
                                medJson["media_id"] = med.getId();
                                medJson["media_type"] = med.getMediaType();
                                medJson["created_at"] = med.getCreatedAt();
                                medJson["media_path"] = med.getMediaPath();
                                medJson["meta_path"] = med.getMetaPath();
                                medJson["message_id"] = med.getMessageId();
                                mediaJson.push_back(medJson);
                            }
                            messageJson["media"] = mediaJson;
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
            else if (request["action"] == "new_private_chat")
            {
                if (!currentUser.has_value()) {
                    response["status"] = "error";
                    response["message"] = "User not logged in.";
                }
                else
                {
                    int chatId;
                    std::string chatName = "private";
                    int requestedUserId = request["user_id"];
                    if (currentUser->getId() == requestedUserId)
                    {
                        chatName = reinterpret_cast < const char*>(u8"Заметки");
                    }
                    if (dbManager.createChat(chatName, chatId) == DatabaseManager::StatusCode::SUCCESS)
                    {
                        response["status"] = "success";
                        response["chat_id"] = chatId;
                        response["created_at"] = DatabaseManager::getCurrentTime();
                        response["chat_name"] = chatName;

                        int requestedUserId = request["user_id"];
                        dbManager.addChatMember(chatId, requestedUserId);
                        dbManager.addChatMember(chatId, currentUser->getId());
                        broadcastMessage(response, { requestedUserId, currentUser->getId() });


                        json userJson;
                        userJson["action"] = "new_chat_member";
                        userJson["id"] = currentUser->getId();
                        userJson["username"] = currentUser->getUsername();
                        userJson["created_at"] = currentUser->getCreatedAt();
                        userJson["last_login"] = currentUser->getLastLogin().value_or("");
                        userJson["email"] = currentUser->getEmail();
                        userJson["profile_picture"] = currentUser->getProfilePicture();
                        userJson["bio"] = currentUser->getBio();
                        userJson["first_name"] = currentUser->getFirstName();
                        userJson["last_name"] = currentUser->getLastName();
                        userJson["date_of_birth"] = currentUser->getDateOfBirth();
                        userJson["chat_id"] = chatId;
                        userJson["role"] = "member";
                        userJson["joined_at"] = DatabaseManager::getCurrentTime();

                        broadcastMessageToChat(chatId, userJson);

                        auto user = dbManager.GetUser(requestedUserId);
                        if (user) {
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
                            broadcastMessageToChat(chatId, userJson);
                        }
                        
                        
                        
                       
                        continue;
                    }
                    else
                    {
                        response["status"] = "error";
                        response["message"] = "Cannot create chat.";
                    }
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
                            if (dbManager.updateUserProfile(request["user_id"], "username", request["username"]) == DatabaseManager::StatusCode::_ERROR)
                            {
                                response["status"] = "error";
                                response["message"] = "Username error.";
                            }
                            else
                            {
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
                                request["action"] = "update_user";
                                broadcastMessage(request.dump());

                                currentUser = dbManager.GetUser(currentUser->getId());
                            }

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
                        userJson["last_login"] = DatabaseManager::getCurrentTime();
                        userJson["email"] = user->getEmail();
                        userJson["profile_picture"] = user->getProfilePicture();
                        userJson["bio"] = user->getBio();
                        userJson["first_name"] = user->getFirstName();
                        userJson["last_name"] = user->getLastName();
                        userJson["date_of_birth"] = user->getDateOfBirth();
                        userJson["chat_id"] = 1;
                        userJson["role"] = "member";
                        userJson["joined_at"] = DatabaseManager::getCurrentTime();
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
            else if (request["action"] == "logout")
            {
                currentUser = std::nullopt;
                response["status"] = "success";
                response["message"] = "Loguot successful.";
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

        sendResponse(client.get(),response);
    }
    if (currentUser!=std::nullopt)
    {
        clientSockets.erase(currentUser->getId());
    }
}

void ChatServer::broadcastMessage(const json& messageJson, std::vector<int> usersId)
{
    for (const auto& [id, socket] : clientSockets)
    {
        auto it = std::remove_if(usersId.begin(), usersId.end(), [&](int userId) {
            if (id == userId)
            {
                std::string messageData = messageJson.dump();
                sf::Packet packet;
                packet << messageData;
                socket->send(packet);
                return true;
            }
            return false;
            });

        
        usersId.erase(it, usersId.end());
        if (usersId.empty())break;
    }
}

void ChatServer::broadcastMessageToChat(int chatId, const json& messageJson) {
    auto chatMembers = dbManager.getChatMembers(chatId);
    spdlog::info(messageJson.dump(4));


    sf::Packet packet;

    std::string messageData = messageJson.dump();
    packet << messageData;

    pushMediaToPacket(packet, messageJson);


    for (const auto& member : chatMembers) {
        int memberId = member.getUserId();

        if (clientSockets.find(memberId) != clientSockets.end()) {
            auto client = clientSockets[memberId];
            if (client->send(packet) != sf::Socket::Status::Done) {
                spdlog::error("Failed to send message to user {} in chat {}", memberId, chatId);
            }
            else {
                spdlog::info("Message sent to user {} in chat {}", memberId, chatId);
            }
        }
    }
}

void ChatServer::sendResponse(sf::TcpSocket* client, const json& response)
{
    sf::Packet responsePacket;
    responsePacket << response.dump();
    pushMediaToPacket(responsePacket, response);
    client->send(responsePacket);
}

void ChatServer::pushMediaToPacket(sf::Packet& packet, const json& messageJson)
{
    std::vector<std::vector<char>> mediaDataList;
    std::vector<std::vector<char>> metaDataList;

    if (messageJson.contains("media")) {
        for (const auto& mediaItem : messageJson["media"]) {
            std::string mediaPath = mediaItem["media_path"];
            std::string metaPath = mediaItem["meta_path"];

            try {

                mediaDataList.push_back(readFile(mediaPath));
                metaDataList.push_back(readFile(metaPath));
            }
            catch (const std::exception& e) {
                spdlog::error("Error reading media files: {}", e.what());
                return;
            }
        }
    }

    UINT32 mediaCount = static_cast<UINT32>(mediaDataList.size());
    packet << mediaCount;


    for (size_t i = 0; i < mediaDataList.size(); ++i) {
        const auto& mediaData = mediaDataList[i];
        const auto& metaData = metaDataList[i];
        size_t mediaSize = mediaData.size();
        packet << static_cast<UINT32>(mediaSize);
        if (mediaSize > 0) {
            packet.append(mediaData.data(), mediaSize);
        }
        size_t metaSize = metaData.size();
        packet << static_cast<UINT32>(metaSize);
        if (metaSize > 0) {
            packet.append(metaData.data(), metaSize);
        }
    }
}

void ChatServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex);
    for (const auto& [userId, client] : clientSockets) {
        sf::Packet packet;
        packet << message;
        if (client->send(packet) != sf::Socket::Status::Done) {
            spdlog::error("Failed to send message to client: {}", client->getRemoteAddress()->toString());
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


std::shared_ptr<Media> ChatServer::generatePlot(const json& plotData, int messageId)
{
    namespace fs = std::filesystem;
    double xStep = plotData["x_step"];
    double xBegin = plotData["x_begin"];
    double xEnd = plotData["x_end"];
    std::string expression = plotData["expression"];
    std::pair< std::vector<double>, std::vector<double>> dots;
    std::map<std::string, double> coefficients;
    if (plotData.contains("coefficients")) {
        for (const auto& [key, value] : plotData["coefficients"].items()) {
            coefficients[key] = value.get<double>();
        }
    }
    calculate(dots, expression, xBegin, xEnd, xStep, coefficients);
    fs::path dirPath = "media/charts";
    fs::path filePath = dirPath / ("dots"+std::to_string(messageId)+ std::to_string(std::time(nullptr)) + ".bin");
    fs::path metaFilePath = dirPath / ("meta" + std::to_string(messageId) + std::to_string(std::time(nullptr)) + ".json");

    if (!fs::exists(dirPath)) {
        fs::create_directories(dirPath);
    }

    std::ofstream outFile(filePath.string(), std::ios::binary);
    if (outFile.is_open()) {
        size_t size = dots.first.size();
        outFile.write(reinterpret_cast<const char*>(dots.first.data()), size * sizeof(double));
        outFile.write(reinterpret_cast<const char*>(dots.second.data()), size * sizeof(double));
        outFile.close();
    }
    else {
        spdlog::error("Cannot open {} ", filePath.string());
    }

    nlohmann::json metaJson;
    metaJson["expression"] = expression;
    metaJson["x_step"] = plotData["x_step"];
    metaJson["x_begin"] = plotData["x_begin"];
    metaJson["x_end"] = plotData["x_end"];
    metaJson["coefficients"] = plotData["coefficients"];
    metaJson["points_file_path"] = filePath.string();
    std::ofstream metaFile(metaFilePath.string());
    if (metaFile.is_open()) {
        metaFile << metaJson.dump(4);
        metaFile.close();
    }
    else {
        spdlog::error("Cannot open {} ", metaFilePath.string());
    }
    auto media = std::make_shared<Media>(0, messageId, "plot", filePath.string(), metaFilePath.string(), DatabaseManager::getCurrentTime(), (dots.first.size() * 2 + sizeof(dots.first.size())));
    dbManager.insertMedia(*media);
    return media;
}
