#pragma once
#include <fstream>
#include <filesystem>
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <spdlog/spdlog.h>
#include <data/SQLModels.hpp>

using json = nlohmann::json;

class ServerClient {
public:
    explicit ServerClient(const std::optional<sf::IpAddress>& serverAddress, unsigned short port);
    bool connect();
    bool sendRequest(const json& request);
    bool receiveResponse(sf::Packet& packet);

    const std::optional < sf::IpAddress>& getServerAddress() const { return serverAddress; }
    unsigned short getServerPort() const { return serverPort; }

private:
    sf::TcpSocket socket;
    std::optional<sf::IpAddress> serverAddress;
    unsigned short serverPort;
    std::queue<json> responseQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
};