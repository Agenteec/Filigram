#include <ServerClient.h>

ServerClient::ServerClient(const sf::IpAddress& serverAddress, unsigned short port)
    : serverAddress(serverAddress), serverPort(port), running(true) {}

bool ServerClient::connect() {
    return socket.connect(serverAddress, serverPort) == sf::Socket::Done;
}

bool ServerClient::sendRequest(const json& request) {
    sf::Packet packet;
    packet << request.dump();
    return socket.send(packet) == sf::Socket::Done;
}

bool ServerClient::receiveResponse(sf::Packet& packet) {
    if (socket.receive(packet) == sf::Socket::Done) {
        return true;
    }
    return false;
}