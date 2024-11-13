#include <MainWindow.h>


MainWindow::MainWindow() :
    window(new sf::RenderWindow(sf::VideoMode(800, 600), L"Filigram", sf::Style::Default)),
    onChat(false),
    onRegister(false),
    onLogin(true),
    rememberMe(false),
    onChatInfo(false),
    client(nullptr),
    connectionStatus(Disconnected),
    currentChat(nullptr),
    scrollToBottomChat(true),
    moveToNewChat(false),
    onProfileEditor(false),
    onPlotEditor(false)
{
    init();

}

MainWindow::~MainWindow() {
    stop();
    delete window;
}

void MainWindow::start() {
    clientThread = std::thread([this]() {
        client = new ServerClient(serverIp, serverPort);

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
                        if (currentUser.isLoad)
                        {
                            sendLoginRequest(currentUser.username,currentUser.password);
                        }
                    }
                    else
                    {
                        spdlog::warn("Failed to connect to server: {}:{}", client->getServerAddress().toString(), client->getServerPort());
                        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
                        
                        
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
            sf::Packet response;
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
    const std::string configFilePath = "cConfig.json";
    json config;

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
    if (config.contains("host")){serverIp = config["host"].get<std::string>();}
    if (config.contains("port")){serverPort = config["port"].get<unsigned short>();}
    if (config.contains("timeout")){timeout = config["timeout"].get<int>();}



    sendMessageTexture.loadFromFile("Assets/images/send.png");
    if (sodium_init() < 0)spdlog::error("Sodium init failed");
    key = std::vector<unsigned char>(crypto_secretbox_KEYBYTES);
    for (size_t i = 0; i < crypto_secretbox_KEYBYTES; i++)
        key[i] = static_cast<unsigned char>(sin(i) * i);

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
    chatInfoWindow(onChatInfo);
    profileEditorWindow(onProfileEditor);
    plotEditorWindow(onPlotEditor);
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
    if (ImGui::InputText("##Password", &currentUser.password, ImGuiInputTextFlags_Password))
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
        if (currentUser.username.empty() || currentUser.password.empty()) {

            
        }
        else
        {
            sendLoginRequest(currentUser.username, currentUser.password);

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

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.25f, 100.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGui::Begin(cu8("##UserProfile"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoNavFocus);
    if (ImGui::IsWindowFocused())
    {
        onChatInfoFocus = false;onProfileEditorFocus = false;onPlotEditorFocus = false;
    }
    auto user = userList[currentUser.id];
    if (user)
    {
        if (ImGui::ImageButton((void*)reinterpret_cast<ImTextureID>(user->getProfilePictureTexture()->getNativeHandle()), ImVec2(65, 65)))
        {
            if (!onProfileEditor)
            {
                tempUser.bio = user->getBio();
                tempUser.dateOfBirth = user->getDateOfBirth();
                tempUser.email = user->getEmail();
                tempUser.firstName = user->getFirstName();
                tempUser.id = user->getId();
                tempUser.lastName = user->getLastName();
                tempUser.status = user->getStatus();
                tempUser.profilePicture = user->getProfilePicture();
                tempUser.username = user->getUsername();


                onProfileEditor = true;
            }


        }
        ImGui::SameLine();
        ImGui::Text("%s", user->getUsername().c_str());
    }

    

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.25f, window->getSize().y - 100.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 100.0f), ImGuiCond_Always);

    ImGui::Begin(cu8("##ChatList"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoNavFocus);
    if (ImGui::IsWindowFocused())
    {
        onChatInfoFocus = false;onProfileEditorFocus = false;onPlotEditorFocus = false;
    }
    for (auto& [chatId, chat] : chatList) {
        std::string chatName = chat->getChatName();
        if (chat->getChatType() == "private")
        {
            for (const auto& [memberUserId, member] : chat->getMembers())
            {
                if (memberUserId != currentUser.id)
                {
                    chatName = member->user->getUsername();
                }
            }
        }
        bool isSelected = (currentChat && currentChat->getId() == chatId);

        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.8f, 0.5f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
        }
       
        if (ImGui::Selectable(chatName.c_str(), isSelected)) {
            currentChat = chat;
            onChat = true;
        }

        ImGui::PopStyleColor();
    }

    ImGui::End();
}

void MainWindow::chatImWindow(bool isOpen) {
    if (!isOpen || !currentChat) return;

    float winW = window->getSize().x * 0.75f;
    float winPosX = window->getSize().x * 0.25f;
    float totalHeight = window->getSize().y;

    const float chatInfoHeight = 45.0f;
    const float tbwHeight = 45.0f;
    float chatHeight = totalHeight - chatInfoHeight - tbwHeight;

    ImGui::SetNextWindowSize(ImVec2(winW, chatInfoHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(winPosX, 0), ImGuiCond_Always);
    std::string chatName = currentChat->getChatName();
    if (currentChat->getChatType() == "private")
    {
        for (const auto& [memberUserId, member] : currentChat->getMembers())
        {
            if (memberUserId != currentUser.id)
            {
                chatName = member->user->getUsername();
            }
        }
    }
    if (ImGui::Begin("##ChatInfo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoNavFocus)) {
        ImGui::Text(chatName.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Информация")) {
            onChatInfo = !onChatInfo;
        }
        if (ImGui::IsWindowFocused())
        {
            onChatInfoFocus = false;onProfileEditorFocus = false;onPlotEditor = false;
        }
    }
    ImGui::End();
    ImGui::SetNextWindowSize(ImVec2(winW, chatHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(winPosX, chatInfoHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("##Chat"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNavFocus);
    if (ImGui::IsWindowFocused())
    {
        onChatInfoFocus = false;onProfileEditorFocus = false;
    }
    int lastSenderId = -1;
    for (const auto& message : currentChat->getMessages()) {
        auto sender = message.second->user;

        if (!sender) continue;
        for (auto& [mediaId, media] : message.second->getMedia()) {
            if (media->getMediaType() == "plot") {
                
                double* xData = reinterpret_cast< double*>( media->data.data());
                double* yData = reinterpret_cast< double*>( media->data.data()+ (media->data.size()/2));
                std::string name = "##Plot" + std::to_string(mediaId);
                if (ImPlot::BeginPlot(name.c_str())) {
                    ImPlot::PlotLine(media->metaData["expression"].get<std::string>().c_str(), xData, yData, media->data.size() / 2 / sizeof(double));
                    ImPlot::EndPlot();
                }
            }
        }
        if (lastSenderId != sender->getId()) {
            ImGui::Image((void*)reinterpret_cast<ImTextureID>(sender->getProfilePictureTexture()->getNativeHandle()), ImVec2(40, 40));
            ImGui::SameLine();
            auto& nameColor =  sender->nameColor;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(nameColor.r/255.f, nameColor.g/255.f, nameColor.b/255.f, nameColor.a/255.f));
            ImGui::Text("%s", sender->getUsername().c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", message.second->getCreatedAt().c_str());
            lastSenderId = sender->getId();
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("MessageContextMenu");
        }

        if (ImGui::BeginPopup("MessageContextMenu")) {
            if (ImGui::MenuItem("Ответить")) {
                //prepareReply(message.second);
            }
            if (ImGui::MenuItem("Переслать")) {
                //forwardMessage(message.second);
            }
            ImGui::EndPopup();
        }
        ImGui::TextWrapped("%s", message.second->getMessageText().c_str());

    }

    scrollToBottomChatLevel = ImGui::GetScrollY() / ImGui::GetScrollMaxY();
    if (scrollToBottomChat) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottomChat = false;
    }
    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::SetNextWindowSize(ImVec2(winW, tbwHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(winPosX, chatInfoHeight + chatHeight), ImGuiCond_Always);
    ImGui::Begin("##TBW", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavFocus);
    if (ImGui::IsWindowFocused())
    {
        onChatInfoFocus = false;onProfileEditorFocus = false; onPlotEditor = false;
    }
    
    float inputWidth = winW - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("✉").x - ImGui::CalcTextSize("☄").x*2 - 2 * ImGui::GetStyle().FramePadding.x;
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputText("##MessageTB", &newMessage);

    if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (!newMessage.empty()) {
            sendMessage(newMessage);
            newMessage.clear();
            scrollToBottomChat = true;
        }
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::SameLine();

    if (ImGui::Button("✉")) {
        if (!newMessage.empty()) {
            sendMessage(newMessage);
            newMessage.clear();
            scrollToBottomChat = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("☄")) {
        ImGui::OpenPopup("attachmentPopup");
    }


    if (ImGui::BeginPopup("attachmentPopup")) {
        if (ImGui::MenuItem("График")) {
            onPlotEditor = true;
        }
        if (ImGui::MenuItem("...")) {

        }
        ImGui::EndPopup();
    }
    ImGui::End();
}

void MainWindow::chatInfoWindow(bool isOpen) {
    if (!isOpen || !currentChat) return;

    float winW = 400.0f;
    float winH = 400.0f;
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((window->getSize().x - winW) / 2, (window->getSize().y - winH) / 2), ImGuiCond_Always);
    if (!onChatInfoFocus)
    {
        onChatInfoFocus = !onChatInfoFocus;
        ImGui::SetNextWindowFocus();
    }
    
    if (ImGui::Begin(cu8("Информация"), &onChatInfo, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus)) {
        ImGui::Text("%s", currentChat->getChatName().c_str());
        ImGui::Text("Участников: %d", static_cast<int>(currentChat->getMembers().size()));
        ImGui::Separator();

        for (const auto& [id, member] : currentChat->getMembers()) {
            auto user = member->user;
            if (!user) continue;

            ImGui::PushID(id);
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.3f, 0.7f));

            if (ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(winW - 20, 60))) {
                
            }

            ImGui::PopStyleColor();

            ImGui::SameLine(10.0f);
            ImGui::Image((void*)reinterpret_cast<ImTextureID>(user->getProfilePictureTexture()->getNativeHandle()), ImVec2(50, 50));

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::Text("%s", user->getUsername().c_str());
            ImGui::Text("Последний вход: %s", user->getLastLogin().value_or("-").c_str());
            ImGui::EndGroup();


            if (ImGui::BeginPopupContextItem(("UserContextMenu" + std::to_string(id)).c_str())) {
                if (ImGui::MenuItem("Написать сообщение")) {
                    openDirectMessage(user->getId());
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}

void MainWindow::openDirectMessage(int userId)
{

    onChatInfo = false;
    bool isNotes = false;
    int chatId = -1;
    if (userId == currentUser.id)
    {
        isNotes = true;
    }

    for (const auto& [_chatId, chat] : chatList)
    {
        if (chat->getChatType() == "private")
        {
            if (isNotes && chat->getMembers().size() >= 2)
                continue;
            for (auto& [memberId, member] : chat->getMembers())
            {
                if (memberId == userId)
                {
                    chatId = _chatId;
                    break;
                }
            }
        }
        if (chatId != -1)break;
    }
    
    
    if (chatId != -1)
        currentChat = chatList[chatId];
    else
    {
        moveToNewChat = true;
        sendNewPrivateChatRequest(userId);
    }
}

void MainWindow::profileEditorWindow(bool isOpen)
{
    if (!isOpen) return;
    if (!onProfileEditorFocus)
    {
        ImGui::SetNextWindowFocus();
        onProfileEditorFocus = !onProfileEditorFocus;
    }

    if (ImGui::Begin(cu8("Редактор профиля"), &onProfileEditor, ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_AlwaysAutoResize
        )) {

        ImGui::InputText(cu8("Имя пользователя"), &tempUser.username);
        ImGui::InputText(cu8("Электронная почта"), &tempUser.email);
        ImGui::InputTextMultiline(cu8("Биография"), &tempUser.bio);
        ImGui::InputText(cu8("Имя"), &tempUser.firstName);
        ImGui::InputText(cu8("Фамилия"), &tempUser.lastName);
        //ImGui::InputText(cu8("Дата рождения"), &tempUser.dateOfBirth);
        //ImGui::InputText(cu8("Профильная картинка"), &tempUser.profilePicture);

        const char* statuses[] = { "online", "offline", "do not disturb" };
        int currentStatusIndex = 0;
        for (int i = 0; i < 3; ++i) {
            if (tempUser.status == statuses[i]) {
                currentStatusIndex = i;
                break;
            }
        }
        if (ImGui::Combo(cu8("Статус"), &currentStatusIndex, statuses, IM_ARRAYSIZE(statuses))) {
            tempUser.status = statuses[currentStatusIndex];
        }

        if (ImGui::Button(cu8("Сохранить"))) {
            onProfileEditor = false;
            sendUpdateUserInfoRequest();
        }

        ImGui::End();
    }
}

void MainWindow::plotEditorWindow(bool isOpen)
{
    if (!isOpen) return;
    
    static std::map<std::string, double> coefficients;
    static char lastCoefficient = 'a';
    static std::pair<std::vector<double>, std::vector<double>> dots = {};
    static std::string expression = "sin(x)";
    static std::string expressionEx = "";
    static bool expressionChangedEx = false;
    static bool expressionChanged = true;
    static double xBegin = -3.141592653579 * 5.;
    static double xEnd = 3.141592653579 * 5.;
    static double xStep = 0.1;


    if (!onPlotEditorFocus)
    {
        onPlotEditorFocus = true;
        ImGui::SetNextWindowFocus();
    }
    if (ImGui::Begin("График", &onPlotEditor, ImGuiWindowFlags_NoCollapse))
    {
        auto wSize = ImGui::GetWindowSize();
        ImGui::BeginGroup();
        if (ImGui::InputText("Выражение", &expression)) {
            expressionChanged = true;
        }
        ImGui::PushItemWidth(wSize.x*0.2);
        if (ImGui::InputDouble("Начало", &xBegin, 0.01, 1.0))expressionChanged = true;
        ImGui::SameLine();
        if(ImGui::InputDouble("Конец", &xEnd, 0.01, 1.0)) expressionChanged = true;
        ImGui::SameLine();
        if (ImGui::InputDouble("Шаг", &xStep, 0.01, 1.0))
        {
            if (xStep <= 0.0) {
                xStep = 0.01;
            }
            expressionChanged = true;
        }
        ImGui::PopItemWidth();


        if (expressionChanged)
        {
            try {
                dots.first.clear();
                dots.second.clear();
                double previewXStep = xStep;
                while ((xEnd - xBegin) / previewXStep > 10000)
                {
                    previewXStep += 0.1;
                }
                calculate(dots, expression, xBegin, xEnd, previewXStep, coefficients);
                expressionChanged = false;
                expressionChangedEx = false;
            }
            catch (const mu::Parser::exception_type& e) {
                expressionEx = e.GetMsg();
                expressionChangedEx = true;
            }
        }

        if (expressionChangedEx)
        {
            ImGui::Text("%s", expressionEx.c_str());
        }

        if (ImPlot::BeginPlot("Предварительный просмотр",ImVec2( wSize.x*0.7f,-1))) {
            ImPlot::PlotLine("f(x)", dots.first.data(), dots.second.data(), dots.first.size());
            ImPlot::EndPlot();
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        if (ImGui::BeginListBox("Коэффициенты", ImVec2(wSize.x * 0.3f, wSize.y*0.7f)))
        {
            ImGui::Text("Коэффициенты:");
            for (auto& [name, value] : coefficients)
            {
                ImGui::PushItemWidth(200);
                if (ImGui::InputDouble(name.c_str(), &value, 0.01, 1.0))
                {
                    expressionChanged = true;
                }
                ImGui::PopItemWidth();
            }

            if (ImGui::Button("+", ImVec2(30, 30)))
            {
                if (lastCoefficient < 'x')
                {
                    coefficients[{lastCoefficient}] = 0;
                    lastCoefficient++;
                    expressionChanged = true;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("-", ImVec2(30, 30)))
            {
                if (!coefficients.empty())
                {
                    coefficients.erase(--coefficients.end());
                    lastCoefficient--;
                    expressionChanged = true;
                }
            }

            ImGui::EndListBox();
        }
        if (ImGui::Button("Сброс"))
        {
            expression = "sin(x)";
            coefficients.clear();
            lastCoefficient = 'a';
            expressionEx = "";
            expressionChangedEx = false;
            expressionChanged = true;
            xBegin = -3.141592653579 * 5.;
            xEnd = 3.141592653579 * 5.;
            xStep = 0.1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Отправить"))
        {
            if (!expressionChangedEx && !expression.empty())
            {
                onPlotEditor = false;
                sendMessage(newMessage, PlotData(xStep, xBegin, xEnd, coefficients, expression));
                newMessage.clear();
            }
        }
        ImGui::EndGroup();
        ImGui::End();
    }
}

void MainWindow::sendMessage(const std::string& message) {
    json request;
    request["action"] = "send_message";
    request["message"] = message;
    request["chat_id"] = currentChat->getId();
    request["message_type"] = "default";

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::sendMessage(const std::string& message, const PlotData& plotData)
{
    json request;
    request["action"] = "send_message";
    request["message"] = message;
    request["chat_id"] = currentChat->getId();
    request["message_type"] = "plot";
    json data;
    data["x_step"] = plotData.xStep;
    data["x_begin"] = plotData.xBegin;
    data["x_end"] = plotData.xEnd;
    data["expression"] = plotData.expression;
    json jCoefficients;
    for (const auto& [name, value] : plotData.coefficients)jCoefficients[name] = value;
    data["coefficients"] = jCoefficients;
    request["plot_data"] = data;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::sendUpdateUserInfoRequest()
{

    json request;
   
    request["action"] = "update_user_info";
    request["user_id"] = currentUser.id;
    request["username"] = tempUser.username;
    request["user_email"] = tempUser.email;
    request["user_profilePicture"] = tempUser.profilePicture;
    request["user_bio"] = tempUser.bio;
    request["user_status"] = tempUser.status;
    request["user_firstName"] = tempUser.firstName;
    request["user_lastName"] = tempUser.lastName;
    request["user_dateOfBirth"] = tempUser.dateOfBirth;


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

void MainWindow::sendNewPrivateChatRequest(int UserId)
{
    json request;
    request["action"] = "new_private_chat";
    request["user_id"] = UserId;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();

}

void MainWindow::sendGetUserChatsRequest() {
    json request;
    request["action"] = "get_user_chats";
    request["user_id"] = currentUser.id;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::sendGetMediaDataRequest()
{
    //Нужно будет сделать защиту от несанкционированного доступа

    json mediaIdsJson = json::array();
    for (auto id : mediaLoadIdVec)mediaIdsJson.push_back(id);
    mediaLoadIdVec.clear();
    json request;
    request["action"] = "get_media_data";
    request["media_ids"] = mediaIdsJson;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push(request);
    }
    cv.notify_one();
}

void MainWindow::GetMessageMedia(sf::Packet& packet, std::shared_ptr<Message> newMessage, const json& response)
{
    sf::Uint32 mediaCount;
    packet >> mediaCount;

    for (size_t i = 0; i < mediaCount; i++) {
        sf::Uint32 mediaSize;
        packet >> mediaSize;

        std::vector<char> mediaData(mediaSize);
        const char* data = reinterpret_cast<const char*>(packet.getData());
        size_t readPosition = packet.getReadPosition();

        if (mediaSize > 0) {
            for (size_t j = 0; j < mediaSize; j++) {
                mediaData[j] = data[readPosition + j];
            }
            while (packet.getReadPosition() < readPosition + mediaSize)
            {
                sf::Uint8 temp;
                packet >> temp;
            }

        }

        sf::Uint32 metaSize;
        packet >> metaSize;

        std::vector<char> metaData(metaSize);
        if (metaSize > 0) {
            data = reinterpret_cast<const char*>(packet.getData());
            readPosition = packet.getReadPosition();

            for (size_t j = 0; j < metaSize; j++) {
                metaData[j] = data[readPosition + j];
            }
            while (packet.getReadPosition() < readPosition + metaSize)
            {
                sf::Uint8 temp;
                packet >> temp;
            }
        }

        json mediaItem = response["media"][i];
        spdlog::info(response.dump());

        spdlog::info(mediaItem.dump());
        auto media = std::make_shared<Media>(
            mediaItem["media_id"],
            newMessage->getId(),
            mediaItem["media_type"],
            mediaItem["media_path"],
            mediaItem["meta_path"],
            mediaItem["created_at"]
        );

        std::string mediaFilePath = mediaItem["media_path"];
        std::ofstream outFile(mediaFilePath, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(mediaData.data(), mediaData.size());
            outFile.close();
            spdlog::info("Saved media to {}", mediaFilePath);
        }
        else {
            spdlog::error("Cannot open file for writing: {}", mediaFilePath);
        }

        std::string metaFilePath = mediaItem["meta_path"];
        json metaJson;
        metaJson = json::parse(std::string(metaData.begin(), metaData.end()));

        std::ofstream metaOutFile(metaFilePath);
        if (metaOutFile.is_open()) {
            metaOutFile << metaJson.dump(4);
            metaOutFile.close();
            spdlog::info("Saved metadata to {}", metaFilePath);
        }
        else {
            spdlog::error("Cannot open metadata file for writing: {}", metaFilePath);
        }
        media->metaData = metaJson;
        media->data = mediaData;

        newMessage->addMedia(media);
    }
}

void MainWindow::processServerResponse(sf::Packet& packet) {
    std::string messageResponse;
    packet >> messageResponse;
    json response;

    try
    {
        response = json::parse(messageResponse);
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Cannot parse json {}", ex.what());
        return;
    }
    spdlog::info(response.dump());

    std::string action = response["action"];
    if (action == "new_message") {
        int chatId = response["chat_id"];
        int messageId = response["message_id"];
        auto chatIt = chatList.find(chatId);

        if (chatIt != chatList.end()) {
            auto chat = chatIt->second;
            auto newMessage = std::make_shared<Message>(
                messageId,
                chatId,
                response["sender_id"],
                response["message_text"],
                response["timestamp"]
            );

            newMessage->chat = chat;
            auto userIt = userList.find(response["sender_id"]);
            if (userIt != userList.end()) {
                newMessage->user = userIt->second;
            }

            GetMessageMedia(packet, newMessage, response);

            chat->addMessage(newMessage);
            messageList[messageId] = newMessage;

            if (currentChat && currentChat->getId() == chatId && scrollToBottomChatLevel > 0.95f) {
                scrollToBottomChat = true;
            }
        }
    }
    else if (action == "ping" && response["status"] == "ping")
    {
        sendPingRequest("pong");
    }
    else if (action == "new_chat_member")
    {
        int userId = response["id"];
        auto user = std::make_shared<User>(
            userId,
            response["username"],
            "",
            response["created_at"],
            response.value("last_login", ""),
            response.value("email", ""),
            response.value("profile_picture", ""),
            response.value("bio", ""),
            response.value("first_name", ""),
            response.value("last_name", ""),
            response.value("date_of_birth", "")
        );
        if (!userList[userId])
        {
            user->nameColor = getColorFromString(user->getFirstName() + user->getUsername());
            userList[userId] = user;
        }
        auto chatMember = std::make_shared<ChatMember>(response["chat_id"], response["id"], response["joined_at"]);
        chatMember->user = user;
        if (chatList[response["chat_id"]])
        {
            chatList[response["chat_id"]]->addMember(chatMember);
        }
    }
    else if (action == "new_private_chat")
    {
        if (response["status"] == "success")
        {
            auto chat = std::make_shared<Chat>(response["chat_id"], response["chat_name"], response["created_at"]);
            chatList[chat->getId()] = chat;
            moveToNewChat = false;
            currentChat = chat;
        }
        else {
            spdlog::error("Login error: {}", response["message"].get<std::string>());
        }
    }
    else if (action == "update_user_info")
    {
        spdlog::info(response["message"]);
    }
    else if (action == "update_user")
    {
        int userId = response["user_id"];
        if (userList[userId])
        {
            if (!response["username"].empty())
                userList[userId]->setUsername( response["username"]);
            if (!response["user_email"].empty())
                userList[userId]->setEmail(response["user_email"]);
            if (!response["user_profile_picture_data"].empty())
                userList[userId]->setProfilePicture( response["user_profile_picture"]);
            if (!response["user_bio"].empty())
                userList[userId]->setBio( response["user_bio"]);
            if (!response["user_status"].empty())
                userList[userId]->setStatus( response["user_status"]);
            if (!response["user_first_name"].empty())
                userList[userId]->setFirstName( response["user_first_name"]);
            if (!response["user_last_name"].empty())
                userList[userId]->setLastName(response["user_last_name"]);
            if (!response["user_date_of_birth"].empty())
                userList[userId]->setDateOfBirth( response["user_date_of_birth"]);
        }
    }
    else if(action == "get_user_chats") {
        if (response["status"] == "success") {
            chatList.clear();
            userList.clear();
            messageList.clear();
            chatMemberList.clear();
            currentChat = nullptr;

            if (response.contains("users")) {
                for (const auto& userJson : response["users"]) {
                    int userId = userJson["id"];
                    auto user = std::make_shared<User>(
                        userId,
                        userJson["username"],
                        "",
                        userJson["created_at"],
                        userJson.value("last_login", ""),
                        userJson.value("email", ""),
                        userJson.value("profile_picture", ""),
                        userJson.value("bio", ""),
                        userJson.value("first_name", ""),
                        userJson.value("last_name", ""),
                        userJson.value("date_of_birth", "")
                    );
                    user->nameColor = getColorFromString(user->getFirstName()+user->getUsername());
                    
                    userList[userId] = user;
                }
            }

            for (const auto& chatJson : response["chats"]) {
                if (!chatJson.contains("id") || !chatJson.contains("name") || !chatJson.contains("created_at")) {
                    spdlog::warn("Invalid chat data in response: missing 'id', 'name', or 'created_at'");
                    continue;
                }

                auto chat = std::make_shared<Chat>(
                    chatJson["id"],
                    chatJson["name"],
                    chatJson["created_at"],
                    chatJson.value("last_activity", ""),
                    chatJson.value("chat_type", "private")
                );

                for (const auto& memberJson : chatJson["members"]) {
                    if (!memberJson.contains("user_id")) {
                        spdlog::warn("Invalid member data in response: missing 'user_id'");
                        continue;
                    }

                    int userId = memberJson["user_id"];
                    auto userIt = userList.find(userId);
                    if (userIt == userList.end()) {
                        spdlog::warn("User with id {} not found in userList", userId);
                        continue;
                    }

                    auto member = std::make_shared<ChatMember>(
                        chat->getId(),
                        userId,
                        memberJson.value("joined_at", ""),
                        memberJson.value("role", "member")
                    );

                    member->user = userIt->second;
                    member->chat = chat;
                    chat->addMember(member);
                    chatMemberList.push_back(member);
                }

                for (const auto& msgJson : chatJson["messages"]) {
                    if (!msgJson.contains("message_id") || !msgJson.contains("chat_id") || !msgJson.contains("message_text")) {
                        spdlog::warn("Invalid message data in response");
                        continue;
                    }

                    int messageId = msgJson["message_id"];
                    auto message = std::make_shared<Message>(
                        messageId,
                        msgJson["chat_id"],
                        msgJson["user_id"],
                        msgJson["message_text"],
                        msgJson["created_at"],
                        msgJson.value("status", "sent")
                    );
                    if (msgJson.contains("media")) {
                        for (const auto& medJson : msgJson["media"]) {
                            int mediaId = medJson["media_id"];
                            std::string mediaType = medJson["media_type"];
                            std::string createdAt = medJson["created_at"];
                            std::string mediaPath = medJson["media_path"];
                            std::string metaPath = medJson["meta_path"];
                            
                            int messageId = medJson["message_id"];
                            namespace fs = std::filesystem;
                            if (fs::exists(mediaPath) && fs::exists(metaPath)) {
                                auto mediaItem = std::make_shared<Media>(
                                    mediaId, messageId, mediaType, mediaPath, metaPath, createdAt
                                );
                                mediaItem->data = readFile(mediaPath);
                                auto meta = readFile(metaPath);

                                mediaItem->metaData = json::parse(meta.begin(), meta.end());
                                message->addMedia(mediaItem);
                            }
                            else
                            {
                                mediaLoadIdVec.push_back(mediaId);
                            }


                        }
                    }
                    message->chat = chat;
                    auto userIt = userList.find(msgJson["user_id"]);
                    if (userIt != userList.end()) {
                        message->user = userIt->second;
                    }

                    chat->addMessage(message);
                    messageList[messageId] = message;
                }

                chatList[chat->getId()] = chat;
            }
            if (!mediaLoadIdVec.empty())
            {
                sendGetMediaDataRequest();
            }
            spdlog::info("User chats, members, and messages loaded successfully.");
        }
        else {
            spdlog::error("Failed to load user chats: {}", response["message"].get<std::string>());
        }
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
            currentUser.id = response["user_id"];
            sendGetUserChatsRequest();
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