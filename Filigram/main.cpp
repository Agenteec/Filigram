#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <imgui_stdlib.h>
#include <implot.h>
#include <vector>
#include <muParser.h>
#define cu8(str) reinterpret_cast<const char*>(u8##str)


bool initImgui(sf::RenderWindow& window)
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
int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), L"Filigram");
    window.setVerticalSyncEnabled(true);
    initImgui(window);
    std::string expression = "sin(x)";
    mu::Parser parser;
    double x = 0.;
    parser.DefineVar("x", &x);
    static int dataSize = 100;
    std::vector<double> x_data(dataSize);
    std::vector<double> y_data(dataSize);
    bool expressionChanged;
    bool expressionChangedEx;
    std::string expressionEx;

    sf::Clock time;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            default:
                break;
            }
        }
        sf::Time elapsedTime = time.restart();
        ImGui::SFML::Update(window, elapsedTime);
        ImGui::Begin(cu8("Мы лучше дурова"));

        ImGui::End();

        window.clear(sf::Color(14, 22, 33));
        ImGui::SFML::Render(window);
        window.display();

    }
}