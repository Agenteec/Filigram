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
            if (request["action"] == "ping" && request["status"] == "ping") {
                response["status"] = "pong";
                response["action"] = "ping";
            }
            else if (request["action"] == "login") {
                std::string username = request["username"];
                std::string password = request["password"];
                int userId = -1;

                if (dbManager.loginUser(username, password, userId) == DatabaseManager::StatusCode::SUCCESS) {
                    response["status"] = "success";
                    response["message"] = "Login successful.";
                    currentUser = dbManager.GetUser(userId);
                    clientSockets[userId] = client;
                }
                else {
                    response["status"] = "error";
                    response["message"] = "Login failed.";
                }
            }
            else if (request["action"] == "register") {
                std::string username = request["username"];
                std::string password = request["password"];
                auto status = dbManager.registerUser(username, password);

                if (status == DatabaseManager::StatusCode::SUCCESS) {
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
            else if (request["action"] == "send_message") {
                std::string message = request["message"];
                broadcastMessage(message);
                response["status"] = "success";
                response["message"] = "Message sent.";
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