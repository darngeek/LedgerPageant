#include "window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	Window* window = Window::GetPtr();
	window->Init();

	HWND win = window->Make(hInstance);
	if (win) {
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Window::DeletePtr();
	return 0;
}
