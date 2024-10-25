#include <MainWindow.h>

MainWindow::MainWindow() :
    window(new sf::RenderWindow(sf::VideoMode(800, 600), L"Filigram", sf::Style::Default)),
    onChat(false),
    onRegister(false),
    onLogin(true),
    client(nullptr),
    connectionStatus(Disconnected)
{
    init();
}

MainWindow::~MainWindow() {
    stop();
    delete window;
}

void MainWindow::start() {
    clientThread = std::thread([this]() {
        client = new ServerClient("127.0.0.1", 53000);

        runningNetworking.store(true);
        while (runningNetworking.load()) {
            if (connectionStatus != Connected && runningNetworking.load()) {
                connectionStatus = Connecting;
                while (connectionStatus != Connected && runningNetworking.load())
                {
                    
                    spdlog::info("Try connection to server...");
                    if (client->connect())
                    {
                        connectionStatus = Connected;
                        spdlog::info("Connected to server {}:{}", client->getServerAddress().toString(), client->getServerPort());
                    }
                    else
                    {
                        spdlog::warn("Failed to connect to server: {}:{}", client->getServerAddress().toString(), client->getServerPort());
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        
                        
                    }
                }
                
            }

            json request;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                cv.wait(lock, [this] { return !requestQueue.empty() || !runningNetworking; });
                if (!runningNetworking) break;
                request = requestQueue.front();
                requestQueue.pop();
            }

            if (client->sendRequest(request)) {
                spdlog::info("Request sent successfully.");
            }
            else {
                connectionStatus = Disconnected;
                spdlog::error("Request failed.");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        delete client;
        });

    clientReceiverThread = std::thread([this]() {
        while (!runningNetworking.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (runningNetworking.load()) {
            json response;
            if (client->receiveResponse(response)) {
                processServerResponse(response);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        });

    started = true;
    sf::Clock clock;
    while (window->isOpen() && started) {
        sf::Time elapsedTime = clock.restart();
        update(elapsedTime);
        render(elapsedTime);
    }
}

void MainWindow::stop() {

    runningNetworking.store(false);
    cv.notify_all();
    if (clientThread.joinable()) clientThread.join();
    if (clientReceiverThread.joinable()) clientReceiverThread.join();
}

void MainWindow::init()
{
    if (sodium_init() < 0)spdlog::error("Sodium init failed");
    key = std::vector<unsigned char>(crypto_secretbox_KEYBYTES);
    for (size_t i = 0; i < crypto_secretbox_KEYBYTES; i++)
    {
        key[i] = static_cast<unsigned char>(sin(i) * i);
    }
    loadUser();
    initIMgui(*window);
    window->setVerticalSyncEnabled(true);
    font.loadFromFile("Assets/fonts/Roboto-Regular.ttf");
    logo.loadFromFile("Assets/images/logo.png");
    window->setIcon(202,161, logo.getPixelsPtr());
    shaders["ping"] = std::make_shared<ShaderManager>(Shaders::ping);
}

void MainWindow::loadUser()
{
    std::ifstream file("credentials.enc", std::ios::binary);
    if (!file) {
        currentUser.isLoad = false;
        return;
    }
    file.close();
    PasswordManager::load_credentials("credentials.enc", currentUser.username, currentUser.password, key);
    currentUser.isLoad = true;
}

void MainWindow::update(const sf::Time& elapsedTime)
{
    if (connectionStatus != Connected)
        shaders["ping"]->start();
    else
        shaders["ping"]->stop();
    for (auto& shader : shaders)
        shader.second->update(elapsedTime.asSeconds()*0.1f);

    handleEvent();
    ImGui::SFML::Update(*window, elapsedTime);
}

void MainWindow::render(const sf::Time& elapsedTime)
{
    auto& refWindow = *window;
    refWindow.clear(sf::Color(14, 22, 33));

    //login & register
    registerImWindow(onRegister);
    loginImWindow(onLogin);

    //login & register

    //Chats
    listChatsImWindow(onChat);
    chatImWindow(onChat);
    //Chats
    for (auto& shader:shaders)
        shader.second->draw(refWindow);
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
        case sf::Event::Resized:
            for (auto& shader : shaders)
                shader.second->handleResize(refWindow.getSize().x, refWindow.getSize().y, refWindow);
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
        if (currentUser.username.empty() || password.empty()) {

            
        }
        else
        {
            sendLoginRequest(currentUser.username, password);

        }
        
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
            currentUser.password = password1;
            sendRegistrationRequest(currentUser.username, password1);
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

void MainWindow::listChatsImWindow(bool isOpen) {
    if (!isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.25f, window->getSize().y), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::Begin(cu8("Список чатов"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    for (const auto& chat : chatList) {
        if (ImGui::Selectable(chat.getChatName().c_str())) {
            currentChat = chat;
            onChat = true;
            loadMessagesForChat(currentChat.getId());
        }
    }

    ImGui::End();
}

void MainWindow::chatImWindow(bool isOpen) {
    if (!isOpen || currentChat.getChatName().empty()) return;

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.75f, window->getSize().y), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(window->getSize().x * 0.25f, 0), ImGuiCond_Always);

    ImGui::Begin(cu8("Чат"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    for (const auto& message : currentChatMessages) {
        ImGui::TextWrapped(message.getMessageText().c_str());
    }

    static std::string newMessage;
    ImGui::InputText("Сообщение", &newMessage);
    if (ImGui::Button("Отправить")) {
        if (!newMessage.empty()) {
            sendMessage(newMessage);
            newMessage.clear();
        }
    }

    ImGui::End();
}

void MainWindow::sendMessage(const std::string& message) {
    json request;
    request["action"] = "send_message";
    request["message"] = message;
    request["chat_id"] = currentChat.getId();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::sendRegistrationRequest(const std::string& username, const std::string& password) {
    json request;
    request["action"] = "register";
    request["username"] = username;
    request["password"] = password;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::sendLoginRequest(const std::string& username, const std::string& password) {
    json request;
    request["action"] = "login";
    request["username"] = username;
    request["password"] = password;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::sendPingRequest(const std::string& status)
{
    json request;
    request["action"] = "ping";
    request["status"] = status;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::processServerResponse(const json& response) {
    spdlog::info(response.dump());

    std::string action = response["action"];
    if (action == "new_message") {

        Message newMessage = Message(
            response["message_id"],
            response["chat_id"],
            response["user_id"],
            response["message_text"],
            response["created_at"],
            response["status"]
        );
        messageQueue.push(newMessage);
        if (newMessage.getChatId() == currentChat.getId()) {
            currentChatMessages.push_back(newMessage);
        }
    }
    else if (action == "ping" && response["status"] == "ping")
    {
        sendPingRequest("pong");
    }
    else if (action == "login") {
        if (response["status"] == "success") {
            spdlog::info("Login ok");
            onLogin = false;
            onRegister = false;
            onChat = true;
            if (rememberMe)
            {
                PasswordManager::save_credentials("credentials.enc", currentUser.username, currentUser.password, key);
            }
        }
        else {
            spdlog::error("Login error: {}", response["message"].get<std::string>());
        }
    }
    else if (action == "register") {
        if (response["status"] == "success") {
            spdlog::info("Register ok");
            sendLoginRequest(currentUser.username, currentUser.password);
        }
        else {
            spdlog::error("Register error: {}", response["message"].get<std::string>());
        }
    }
    
}

void MainWindow::loadMessagesForChat(int chatId) {
    
    currentChatMessages.clear();

    for (const auto& message : messagesList) {
        if (message.getChatId() == chatId) {
            currentChatMessages.push_back(message);
        }
    }
}