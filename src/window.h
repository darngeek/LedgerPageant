#pragma once

#include <windows.h>
#include "application.h"

class Window;
static Window* gWindow;

class Window {
public:
	static Window* GetPtr();
	static void DeletePtr();

	Application* GetApplication() {
		return &mApp;
	}

	HWND Make(HINSTANCE instance);

private:
	Window();
	~Window();

	Application mApp;
};
