#include <MainWindow.h>

MainWindow::MainWindow() :
    window(new sf::RenderWindow(sf::VideoMode(800, 600), L"Filigram", sf::Style::Default)),
    onChat(true),
    onRegister(false),
    onLogin(false)
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
    font.loadFromFile("Assets/fonts/Roboto-Regular.ttf");
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
    static bool passwordRed = false;
    static bool passwordRedChanger = false;
    static bool usernameRed = false;
    static bool usernameRedChanger = false;



    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.5f, window->getSize().y * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(window->getSize().x / 2 - window->getSize().x * 0.25f, window->getSize().y / 2 - window->getSize().y * 0.25f), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("##Login"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(cu8("Вход")).x) / 2);
    ImGui::Text(cu8("Вход"));
    ImGui::Separator();

    ImGui::Text(cu8("Имя пользователя:"));
    ImGui::PushItemWidth(-1.0f);
    if(usernameRed)ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.1f, 0.0f, 0.5f));
    if (ImGui::InputText("##Username", &currentUser.username))
    {
        usernameRedChanger = false;
    }
    if (usernameRed)ImGui::PopStyleColor();
    
    ImGui::Text(cu8("Пароль:"));
    ImGui::PushItemWidth(-1.0f);
    if (passwordRed)ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.1f, 0.0f, 0.5f));
    if (ImGui::InputText("##Password", &password, ImGuiInputTextFlags_Password))
    {
        passwordRedChanger = false;
    }
    if (passwordRed)ImGui::PopStyleColor();
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
    usernameRed = usernameRedChanger;
    passwordRed = passwordRedChanger;
}

void MainWindow::registerImWindow(bool isOpen)
{
    if (!isOpen)
        return;

    static std::string password1;
    static std::string password2;
    static bool passwordsDoNotMatch = false;
    static bool passwordIsTooShort = false;
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
    ImGui::InputText("##Password1", &password1, ImGuiInputTextFlags_Password);

    ImGui::Text(cu8("Повторите пароль:"));
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputText("##Password2", &password2, ImGuiInputTextFlags_Password);

    if (passwordsDoNotMatch)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
        ImGui::Text(cu8("Пароли не совпадают"));
        ImGui::PopStyleColor();
    }
    if (passwordIsTooShort)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Text(cu8("Пароль слишком короткий < 8 символов"));
        ImGui::PopStyleColor();
    }


    passwordsDoNotMatch = !password1._Equal(password2);
    


    if (ImGui::Button(cu8("Регистрация"))) {
        passwordIsTooShort = password1.length() < 8;
        if (!passwordIsTooShort && !passwordsDoNotMatch)
        {
            onRegister = false;
            password1.clear();
            password2.clear();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(cu8("Вход"))) {
        onLogin = true;
        onRegister = false;
        password1.clear();
        password2.clear();
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