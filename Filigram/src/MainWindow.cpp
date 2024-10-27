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
    scrollToBottomChat(true)
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
                        if (currentUser.isLoad)
                        {
                            sendLoginRequest(currentUser.username,currentUser.password);
                        }
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

    ImGui::Begin(cu8("##UserProfile"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    if (userList[currentUser.id])
    {
        ImGui::Image((void*)reinterpret_cast<ImTextureID>(userList[currentUser.id]->getProfilePictureTexture()->getNativeHandle()), ImVec2(65, 65));
        ImGui::SameLine();
    }
    ImGui::Text("%s", currentUser.username.c_str());
    

    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(window->getSize().x * 0.25f, window->getSize().y - 100.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 100.0f), ImGuiCond_Always);

    ImGui::Begin(cu8("##ChatList"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    for (auto& chat : chatList) {
        bool isSelected = (currentChat && chat.second->getChatName() == currentChat->getChatName());

        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.8f, 0.5f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
        }

        if (ImGui::Selectable(chat.second->getChatName().c_str(), isSelected)) {
            currentChat = chat.second;
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

    if (ImGui::Begin("##ChatInfo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text(currentChat->getChatName().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Информация")) {
            onChatInfo = !onChatInfo;
        }
    }
    ImGui::End();
    ImGui::SetNextWindowSize(ImVec2(winW, chatHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(winPosX, chatInfoHeight), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin(cu8("##Chat"), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar);

    int lastSenderId = -1;
    for (const auto& message : currentChat->getMessages()) {
        auto sender = message.second->user;

        if (!sender) continue;

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
    ImGui::Begin("##TBW", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    static std::string newMessage;
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

    if (ImGui::Button(cu8("Отправить"))) {
        if (!newMessage.empty()) {
            sendMessage(newMessage);
            newMessage.clear();
            scrollToBottomChat = true;
        }
    }

    ImGui::End();
}

void MainWindow::chatInfoWindow(bool isOpen) {
    if (!isOpen || !currentChat) return;

    float winW = 400.0f;
    float winH = 400.0f;
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((window->getSize().x - winW) / 2, (window->getSize().y - winH) / 2), ImGuiCond_Always);

    if (ImGui::Begin(cu8("Информация"), &onChatInfo, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
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
    int chatId = -1;
    for (auto& [_chatId, chat]: chatList)
    {
        if (chat->getChatType() == "private")
        {
            for (auto& [memberId,member] : chat->getMembers())
            {
                if (memberId == userId)
                {
                    chatId = _chatId;
                    break;
                }
            }
        }
        if (_chatId != -1)break;
    }
    if (chatId != -1)
        currentChat = chatList[chatId];
    else
    {

    }
}


void MainWindow::sendMessage(const std::string& message) {
    json request;
    request["action"] = "send_message";
    request["message"] = message;
    request["chat_id"] = currentChat->getId();

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

void MainWindow::processServerResponse(const json& response) {
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
        user->nameColor = getColorFromString(user->getFirstName() + user->getUsername());
        userList[userId] = user;
    }
    else if (action == "new_private_chat")
    {
        if (response["status"] == "success")
        {

            /*auto chat = std::make_shared<Chat>(response["chat_id"], userList[response[]]);
            Chat(int id, const std::string & chatName, const std::string & createdAt,
                std::optional<std::string> lastActivity = std::nullopt,
                const std::string & chatType = "private")
            ;*/
        }
        else {
            spdlog::error("Login error: {}", response["message"].get<std::string>());
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