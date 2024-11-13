#include <MainWindow.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>
int main()
{
	namespace fs = std::filesystem;
	system("chcp 65001");
	fs::path dirPathC = "media/charts";
	fs::path dirPathI = "media/images";
	if (!fs::exists(dirPathC))fs::create_directories(dirPathC);
	if (!fs::exists(dirPathI))fs::create_directories(dirPathI);


	std::srand(std::time(nullptr));
	MainWindow mWindow;

	mWindow.start();
}