#include "gui.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Создание и запуск GUI
    GUI gui(hInstance);
    gui.Run();
    return 0;
}
