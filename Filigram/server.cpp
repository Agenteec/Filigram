#include <ChatServer.h>


int main(int argc, char* argv[]) {
    ChatServer server(53000);
    server.run();
    return 0;
}
