#include <MainWindow.h>
#include <stdio.h>
int main()
{
	std::srand(std::time(nullptr));
	MainWindow mWindow;

	mWindow.start();
}