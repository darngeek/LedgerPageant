#pragma once

#include <windows.h>
#include "application.h"

class Window;
static Window* gWindow;

class Window {
public:
	static Window* GetPtr();
	static void DeletePtr();

	Application* getApp() { return &mApp; }

    HWND make(HINSTANCE instance);

private:
	Window();
	~Window();

	Application mApp;
};
