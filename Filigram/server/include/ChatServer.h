#pragma once
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>
#include <string>
#include <thread>
#include <memory>
#include <mutex>
#include <vector>
#include <filesystem>
#include <condition_variable>
#include <queue>
#include <spdlog/spdlog.h>
#include <data/SQLModels.hpp>
#include <data/DatabaseManager.h>
#include <set>
#include "../calc/Calc.h"
using json = nlohmann::json;
/**
 * @brief Класс для ретрансляции и обработки данных от клиентов
 * Работает на стеке TCP/IP с форматом данных JSON а также с использованием бинарной передачи данных
 */
class ChatServer {
public:
    static std::vector<char> readFile(const std::string& filePath);
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
    void broadcastMessage(const json& messageJson, std::vector<int> usersId);
    void broadcastMessageToChat(int chatId, const json& messageJson);
    void sendResponse(sf::TcpSocket* client, const json& response);
    void pushMediaToPacket(sf::Packet& packet, const json& messageJson);
    void startPingThread();
    std::shared_ptr<Media> generatePlot(const json& plotData, int messageId);
};