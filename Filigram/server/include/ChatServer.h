#pragma once
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>
#include <string>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <spdlog/spdlog.h>
#include <data/SQLModels.hpp>
#include <data/DatabaseManager.h>

using json = nlohmann::json;

class ChatServer {
public:
    explicit ChatServer(unsigned short port = 53000);
    void run();

private:
    sf::TcpListener listener;
    DatabaseManager dbManager;
    std::queue<std::string> messageQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::unordered_map<int, std::shared_ptr<sf::TcpSocket>> clientSockets;

    void handleClient(std::shared_ptr<sf::TcpSocket> client);
    void broadcastMessage(const std::string& message);
    void startPingThread();
};