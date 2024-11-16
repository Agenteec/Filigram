
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>

std::vector<std::string> OpenImageFileDialog(const char* filter) {
    OPENFILENAME ofn;
    char szFile[260 * 10];

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    std::vector<std::string> files;

    if (GetOpenFileName(&ofn)) {
        char* p = szFile;
        std::string directory = p;
        p += directory.length() + 1;

        while (*p) {
            files.push_back(directory + "\\" + p);
            p += strlen(p) + 1;
        }
    }

    return files;
}

#else
#include <cstdlib>

std::vector<std::string> OpenFileDialog() {
    std::string command = "zenity --file-selection --title='Выберите файлы' --multiple";
    char buffer[128];
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        spdlog::error("Не удалось открыть процесс.");
        return {};
    }

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    std::vector<std::string> files;
    std::istringstream iss(result);
    std::string file;
    while (std::getline(iss, file, '\n')) {
        if (!file.empty()) {
            files.push_back(file);
        }
    }

    return files;
}
#endif

std::vector<std::string>  SelectImageFileAsync() {
#ifdef _WIN32
    return OpenImageFileDialog("Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All Files (*.*)\0*.*\0");
#else
    return OpenFileDialog();
#endif
}