#include <ServerClient.h>

ServerClient::ServerClient(const std::string& serverAddress, unsigned short port)
    : serverAddress(serverAddress), serverPort(port), running(true) {}

bool ServerClient::connect() {
    return socket.connect(serverAddress, serverPort) == sf::Socket::Done;
}

bool ServerClient::sendRequest(const json& request) {
    sf::Packet packet;
    packet << request.dump();
    return socket.send(packet) == sf::Socket::Done;
}

bool ServerClient::receiveResponse(json& response) {
    sf::Packet packet;
    if (socket.receive(packet) == sf::Socket::Done) {
        std::string data;
        packet >> data;
        response = json::parse(data);
        return true;
    }
    return false;
}

void ServerClient::startReceiverThread() {
    receiverThread = std::thread([this]() {
        while (running) {
            json response;
            if (receiveResponse(response)) {
                std::lock_guard<std::mutex> lock(queueMutex);
                responseQueue.push(response);
                cv.notify_one();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        });
    receiverThread.detach();
}