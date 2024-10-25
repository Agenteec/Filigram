#pragma once
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

using json = nlohmann::json;

class ServerClient {
public:
    explicit ServerClient(const std::string& serverAddress, unsigned short port);
    bool connect();
    bool sendRequest(const json& request);
    bool receiveResponse(json& response);
    void startReceiverThread();

    const sf::IpAddress& getServerAddress() const { return serverAddress; }
    unsigned short getServerPort() const { return serverPort; }

private:
    sf::TcpSocket socket;
    sf::IpAddress serverAddress;
    unsigned short serverPort;
    std::queue<json> responseQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::thread receiverThread;
};