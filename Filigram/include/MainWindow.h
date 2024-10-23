#pragma once
#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include <implot.h>
#include <vector>
#include <muParser.h>
#include <data/User.h>

#define cu8(str) reinterpret_cast<const char*>(u8##str)

static bool initImgui(sf::RenderWindow& window)
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
    sf::RenderWindow* window;
    sf::Image logo;
   
    sf::Font font;

    bool started;
    
    bool onChat;

    bool onLogin;
    bool onRegister;


    bool rememberMe;
    User currentUser;


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

   

};