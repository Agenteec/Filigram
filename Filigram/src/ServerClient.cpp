#include <ServerClient.h>

ServerClient::ServerClient(const std::optional<sf::IpAddress>& serverAddress, unsigned short port)
    : serverAddress(serverAddress), serverPort(port), running(true) {}

bool ServerClient::connect() {
    return socket.connect(serverAddress.value(), serverPort) == sf::Socket::Status::Done;
}

bool ServerClient::sendRequest(const json& request) {
    sf::Packet packet;
    packet << request.dump();
    return socket.send(packet) == sf::Socket::Status::Done;
}

bool ServerClient::receiveResponse(sf::Packet& packet) {
    if (socket.receive(packet) == sf::Socket::Status::Done) {
        return true;
    }
    return false;
}