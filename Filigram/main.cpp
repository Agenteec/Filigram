#include <MainWindow.h>
#include <stdio.h>
#include <iostream>
int main()
{
	system("chcp 65001");

	std::srand(std::time(nullptr));
	MainWindow mWindow;

	mWindow.start();
}