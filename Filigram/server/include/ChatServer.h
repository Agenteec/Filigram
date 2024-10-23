#pragma once
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>
#include <data/SQLModels.hpp>
using json = nlohmann::json;

class ChatServer {
public:
    ChatServer(unsigned short port = 53000) {
        if (listener.listen(port) != sf::Socket::Done) {
            spdlog::error("Error starting server on port {}", port);
        }
        spdlog::info("Server started on port {}", port);
    }

    void run();

private:
    sf::TcpListener listener;

    void handleClient(sf::TcpSocket* client);
};

