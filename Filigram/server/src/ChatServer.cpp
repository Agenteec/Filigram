#include  <ChatServer.h>


#pragma region Public methods
void ChatServer::run()
{
    while (true) {
        sf::TcpSocket* client = new sf::TcpSocket();
        if (listener.accept(*client) == sf::Socket::Done) {
            spdlog::info("New client connected! ip: {}", client->getRemoteAddress().toString());
            handleClient(client);
        }
        else
        {
            delete client;
        }
    }
}
#pragma endregion


#pragma region Private methods
void ChatServer::handleClient(sf::TcpSocket* client)
{
    std::thread([this, client]() mutable {
        auto& refClient = *client;

        while (true) {
            sf::Packet packet;
            if (refClient.receive(packet) != sf::Socket::Done) {
                spdlog::info("Client disconnected. ip: {}", refClient.getRemoteAddress().toString());
                break;
            }

            std::string data;
            packet >> data;
            json response;
            try {
                json request = json::parse(data);

                if (request["action"] == "send_message") {
                    std::string message = request["message"];
                    std::string chatId = request["chat_id"];
                    //
                    response["status"] = "success";
                    response["message"] = "Message sent.";
                }
                else if (request["action"] == "login")
                {
                    std::string username = request["username"];
                    std::string password = request["password"];
                    //
                }
                else if (request["action"] == "get_chats")
                {
                    //
                }
                else if (request["action"] == "logout")
                {
                    //
                }
                else if (request["action"] == "register") {
                    std::string username = request["username"];
                    std::string password = request["password"];
                    //

                    response["status"] = "success";
                    response["message"] = "User registered successfully.";
                }
                else {
                    response["status"] = "error";
                    response["message"] = "Unknown action.";
                }
            }
            catch (const json::parse_error& e) {
                spdlog::error("JSON parse error: {}", e.what());
                spdlog::error("JSON parse user: {}",refClient.getRemoteAddress().toString());
                response["status"] = "error";
                response["message"] = "Invalid JSON format.";
            }
            

            std::string responseData = response.dump();
            sf::Packet responsePacket;
            responsePacket << responseData;
            if (refClient.send(responsePacket) != sf::Socket::Done)
                spdlog::error("Failed to send response to client. ip: {}", refClient.getRemoteAddress().toString());
        }
        }).detach();
    delete client;
}
#pragma endregion


