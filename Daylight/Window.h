#pragma once
#include <Windows.h>

class Window
{
public:
	Window(HINSTANCE hInstance, const wchar_t* title, int width, int height);
	~Window();
public:
	HWND GetHandle() const { return hWnd; };

public:
	bool ProcessMessages();

private:
	HWND hWnd = { };
	int  width = { };
	int  Height	 = { };
};

