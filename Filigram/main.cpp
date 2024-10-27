#include <MainWindow.h>
#include <stdio.h>
int main()
{
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	MainWindow mWindow;

	mWindow.start();
}