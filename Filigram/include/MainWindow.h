#pragma once

#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <list>
#include <functional>
#include <muParser.h>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <ServerClient.h>
#include <data/SQLModels.hpp>
#include <data/PasswordManager.hpp>
#include <shaders/Shaders.h>
#include <shaders/ShaderManager.h>



#define cu8(str) reinterpret_cast<const char*>(u8##str)
static sf::Color getRandomColor(int rMin, int rMax, int gMin, int gMax, int bMin, int bMax, int aMin = 255, int aMax = 255) {
    
    int r = rMin + std::rand() % (rMax - rMin + 1);
    int g = gMin + std::rand() % (gMax - gMin + 1);
    int b = bMin + std::rand() % (bMax - bMin + 1);
    int a = aMin + std::rand() % (aMax - aMin + 1);

    return sf::Color(r, g, b, a);
}
static sf::Color getColorFromString(const std::string& str) {
    std::hash<std::string> hash_fn;
    size_t hash = hash_fn(str);

    int r = static_cast<int>(hash % 256);
    int g = static_cast<int>((hash >> 8) % 256);
    int b = static_cast<int>((hash >> 16) % 256);

    return sf::Color(r, g, b);
}
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
        int id;
        std::string password;
        std::string username;
        bool isLoad;
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
    std::vector<unsigned char> key;
    sf::RenderWindow* window;
    sf::Image logo;

    sf::Font font;

    bool started;

    bool onChat;
    bool onChatInfo;
    bool scrollToBottomChat;
    float scrollToBottomChatLevel;
    bool onLogin;
    bool onRegister;

    bool rememberMe;
    CurrentUser currentUser;
    
    std::queue<json> requestQueue;

    std::mutex queueMutex;
    std::condition_variable cv;

    std::map<int, std::shared_ptr<Chat>> chatList;
    std::map<int, std::shared_ptr<User>> userList;
    std::map<int, std::shared_ptr<Message>> messageList;
    std::vector<std::shared_ptr<ChatMember>> chatMemberList;
    std::map<int, Notification> notificationList;
    std::map<int, Media> mediaList;
    std::map<int, Reaction> reactionList;
    std::unordered_map<int, ImTextureID> avatarTextures;
    ImTextureID getProfilePictureTexture(int userId) {
        auto it = avatarTextures.find(userId);
        if (it != avatarTextures.end()) {
            return it->second;
        }
        else {
            
        }
    }

    std::shared_ptr<Chat> currentChat;

    

    std::map<std::string, std::shared_ptr<ShaderManager>> shaders;
public:
    MainWindow();
    ~MainWindow();
    void start();
    void stop();

private:
    void init();

    void loadUser();

    void update(const sf::Time& elapsedTime);
    void render(const sf::Time& elapsedTime);

    void handleEvent();

    void loginImWindow(bool isOpen);
    void registerImWindow(bool isOpen);

    void listChatsImWindow(bool isOpen);
    void chatImWindow(bool isOpen);

    void chatInfoWindow(bool isOpen);
    void openDirectMessage(int userId);

    void sendRegistrationRequest(const std::string& username, const std::string& password);
    void sendLoginRequest(const std::string& username, const std::string& password);
    void sendPingRequest(const std::string& status = "ping");
    void sendNewPrivateChatRequest(int UserId);
    void sendGetUserChatsRequest();
    void sendMessage(const std::string& message);
    void processServerResponse(const json& response);
};
