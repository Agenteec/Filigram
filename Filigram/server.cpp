#include <ChatServer.h>


int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;
    fs::path dirPathC = "media/charts";
    fs::path dirPathI = "media/images";
    if (!fs::exists(dirPathC))fs::create_directories(dirPathC);
    if (!fs::exists(dirPathI))fs::create_directories(dirPathI);
    const std::string configFilePath = "sConfig.json";
    json config;

    unsigned short serverPort = 53000;
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {

        config = {
            {"host", "localhost"},
            {"port", 53000},
            {"timeout", 1000}
        };

        std::ofstream outFile(configFilePath);
        outFile << config.dump(4);
        outFile.close();
        spdlog::info("Configuration file created with default values.");
    }
    else {
        configFile >> config;
        configFile.close();
    }
    //if (config.contains("host")) { serverIp = config["host"].get<std::string>(); }
    if (config.contains("port")) { serverPort = config["port"].get<unsigned short>(); }
    //if (config.contains("timeout")) { serverPort = config["timeout"].get<int>(); }
    ChatServer server(serverPort);
    server.run();
    return 0;
}
