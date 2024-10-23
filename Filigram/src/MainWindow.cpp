#include <MainWindow.h>

MainWindow::MainWindow() :
    window(new sf::RenderWindow(sf::VideoMode(800, 600), L"Filigram", sf::Style::Default)),
    onChat(false),
    onRegister(false),
    onLogin(true)
{
    init();
}

MainWindow::~MainWindow()
{
	delete window;
}

void MainWindow::start()
{
    auto handle = window->getSystemHandle();
    started = true;
    sf::Clock time;
    while (window->isOpen() && started)
    {
        sf::Time elapsedTime = time.restart();
        update(elapsedTime);
        render(elapsedTime);
    }
}

void MainWindow::stop()
{
    started = false;
}

void MainWindow::init()
{
    initImgui(*window);
    window->setVerticalSyncEnabled(true);
    font.loadFromFile("Assets/fonts/Roboto-Light.ttf");
    logo.loadFromFile("Assets/images/logo.png");
    window->setIcon(202,161, logo.getPixelsPtr());

}

void MainWindow::update(const sf::Time& elapsedTime)
{
    handleEvent();
    ImGui::SFML::Update(*window, elapsedTime);
}

void MainWindow::render(const sf::Time& elapsedTime)
{
    auto& refWindow = *window;

    //login & register
    registerImWindow(onRegister);
    loginImWindow(onLogin);
    //login & register

    //Chats
    listChatsImWindow(onChat);
    chatImWindow(onChat);
    //Chats
    refWindow.clear(sf::Color(14, 22, 33));
    ImGui::SFML::Render(refWindow);
    refWindow.display();
}

void MainWindow::handleEvent()
{
    auto& refWindow = *window;

    sf::Event event;
    while (refWindow.pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(event);
        switch (event.type)
        {
        case sf::Event::Closed:
            refWindow.close();
            stop();
            break;
        default:
            break;
        }
    }
}

void MainWindow::loginImWindow(bool isOpen)
{
    if (!isOpen)
        return;

    static std::string password;
    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.5f, window->getSize().y * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(window->getSize().x / 2 - window->getSize().x * 0.25f, window->getSize().y / 2 - window->getSize().y * 0.25f), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("##Login"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(cu8("Вход")).x) / 2);
    ImGui::Text(cu8("Вход"));
    ImGui::Separator();

    ImGui::Text(cu8("Имя пользователя:"));
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##Username", &currentUser.username);

    
    ImGui::Text(cu8("Пароль:"));
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##Password", &password, ImGuiInputTextFlags_Password);

    ImGui::Checkbox(cu8("Запомнить меня"), &rememberMe);
    
    if (ImGui::Button(cu8("Регистрация"))) {
        onLogin = false;
        onRegister = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(cu8("Вход"))) {
        onLogin = false;
    }



    ImGui::End();
    ImGui::PopStyleColor();
}

void MainWindow::registerImWindow(bool isOpen)
{
    if (!isOpen)
        return;

    static std::string password;
    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.5f, window->getSize().y * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(window->getSize().x / 2 - window->getSize().x * 0.25f, window->getSize().y / 2 - window->getSize().y * 0.25f), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("##Register"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(cu8("Регистрация")).x) / 2);
    ImGui::Text(cu8("Регистрация"));
    ImGui::Separator();

    ImGui::Text(cu8("Имя пользователя:"));
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##RUsername", &currentUser.username);


    ImGui::Text(cu8("Пароль:"));
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##Password1", &password, ImGuiInputTextFlags_Password);

    ImGui::Text(cu8("Повторите пароль:"));
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##Password2", &password, ImGuiInputTextFlags_Password);


    if (ImGui::Button(cu8("Регистрация"))) {
        onRegister = false;
    }
    ImGui::SameLine();
    if (ImGui::Button(cu8("Вход"))) {
        onLogin = true;
        onRegister = false;
    }



    ImGui::End();
    ImGui::PopStyleColor();
}

void MainWindow::listChatsImWindow(bool isOpen)
{
    if (!isOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.25f, window->getSize().y), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("Chats"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::End();
    ImGui::PopStyleColor();
}

void MainWindow::chatImWindow(bool isOpen)
{
    if (!isOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.75f, window->getSize().y), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(window->getSize().x * 0.25f, 0), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("Chat"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);



    ImGui::End();
    ImGui::PopStyleColor();
}