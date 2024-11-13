#pragma once
#include <vector>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <list>
#include <functional>
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
#include <imgui_freetype.h>
#include <imgui_internal.h>
#include "../calc/Calc.h"
#include <data/PlotData.hpp>



#define cu8(str) reinterpret_cast<const char*>(u8##str)
static ImFont* emojiFont;
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
static std::vector<char> readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Error reading file: " + filePath);
    }

    return buffer;
}

static bool initIMgui(sf::RenderWindow& window)
{
    if (!ImGui::SFML::Init(window))
        return false;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    static ImWchar ranges[] = { 0x1, 0xFFFF, 0 };
    static ImFontConfig cfg;
    cfg.OversampleH = cfg.OversampleV = 1;
    cfg.MergeMode = true;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    ImFont* robotoFont = io.Fonts->AddFontFromFileTTF("Assets/fonts/Roboto-Regular.ttf", 20.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    if (!robotoFont) {
        std::cerr << "Ошибка загрузки шрифта Roboto!" << std::endl;
        return false;
    }

    ImFont* emojiFont = io.Fonts->AddFontFromFileTTF("Assets/fonts/SEGUIEMJ.TTF", 20.f, &cfg, ranges);
    if (!emojiFont) {
        std::cerr << "Ошибка загрузки шрифта NotoColorEmoji!" << std::endl;
        return false;
    }
    //emojiFont = io.Fonts->AddFontFromFileTTF("Assets/fonts/NotoColorEmoji-Regular.ttf", 20.f, nullptr, io.Fonts->GetGlyphRangesDefault());
    ImGui::SFML::UpdateFontTexture();
    ImPlot::CreateContext();
    return true;
}

class MainWindow
{
    class CurrentUser
    {
    public:
        bool isLoad;
        int id;
        std::string password;
        std::string username;

        std::string email;
        std::string profilePicture;
        std::string bio;
        std::string status;
        std::string firstName;
        std::string lastName;
        std::string dateOfBirth;
    };

    enum ConnectionStatus
    {
        Connected,
        Disconnected,
        Connecting
    };

    sf::IpAddress serverIp = "127.0.0.1";
    unsigned short serverPort = 53000;
    int timeout = 1000;

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
    bool onChatInfoFocus;
    bool scrollToBottomChat;
    float scrollToBottomChatLevel;
    bool onLogin;
    bool onRegister;
    bool onProfileEditor;
    bool onProfileEditorFocus;

    bool onPlotEditor;
    bool onPlotEditorFocus;

    bool moveToNewChat;

    bool rememberMe;
    CurrentUser currentUser;
    CurrentUser tempUser;
    
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
    


    std::shared_ptr<Chat> currentChat;

    

    std::map<std::string, std::shared_ptr<ShaderManager>> shaders;
    sf::Texture sendMessageTexture;

    std::string newMessage;

    std::vector<int> mediaLoadIdVec;
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

    void profileEditorWindow(bool isOpen);
    void plotEditorWindow(bool isOpen);
    void sendUpdateUserInfoRequest();
    void sendRegistrationRequest(const std::string& username, const std::string& password);
    void sendLoginRequest(const std::string& username, const std::string& password);
    void sendPingRequest(const std::string& status = "ping");
    void sendNewPrivateChatRequest(int UserId);
    void sendGetUserChatsRequest();
    void sendGetMediaDataRequest();
    void GetMessageMedia(sf::Packet& packet, std::shared_ptr<Message> newMessage, const json& response);
    void sendMessage(const std::string& message);
    void sendMessage(const std::string& message, const PlotData& plotData);
    void processServerResponse(sf::Packet& packet);
};
