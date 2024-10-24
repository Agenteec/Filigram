#pragma once

#include <vector>
#include <thread>
#include <future>
#include <functional>
#include <muParser.h>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <ServerClient.h>
#include <data/SQLModels.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <list>

#define cu8(str) reinterpret_cast<const char*>(u8##str)

static bool initIMgui(sf::RenderWindow& window)
{
    if (!ImGui::SFML::Init(window))
        return false;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("Assets/fonts/Roboto-Light.ttf", 20.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::SFML::UpdateFontTexture();
    ImPlot::CreateContext();
    return true;
}

class MainWindow
{
    class CurrentUser
    {
    public:
        std::string password;
        std::string username;
    };

    enum ConnectionStatus
    {
        Connected,
        Disconnected,
        Connecting
    };

    std::atomic<bool> runningNetworking;
    std::atomic<int> connectionStatus;

    std::thread clientThread;
    std::thread clientReceiverThread;
    std::mutex mtx;

    ServerClient* client;
    sf::RenderWindow* window;
    sf::Image logo;

    sf::Font font;

    bool started;

    bool onChat;
    bool onLogin;
    bool onRegister;

    bool rememberMe;
    CurrentUser currentUser;
    
    std::queue<json> requestQueue;
    std::queue<Message> messageQueue;

    std::mutex queueMutex;
    std::condition_variable cv;

    std::vector<Chat> chatList;
    Chat currentChat;

    std::list<Message> messagesList;
    std::list<Message> currentChatMessages;

public:
    MainWindow();
    ~MainWindow();
    void start();
    void stop();

private:
    void init();

    void update(const sf::Time& elapsedTime);
    void render(const sf::Time& elapsedTime);

    void handleEvent();

    void loginImWindow(bool isOpen);
    void registerImWindow(bool isOpen);

    void listChatsImWindow(bool isOpen);
    void chatImWindow(bool isOpen);

    void sendRegistrationRequest(const std::string& username, const std::string& password);
    void sendLoginRequest(const std::string& username, const std::string& password);
    void sendMessage(const std::string& message);
    void processServerResponse(const json& response);

    void loadMessagesForChat(int chatId);
};
