#include "Window.h"


// 창에 들어오는 메세지 처리하는 콜백
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

Window::Window(HINSTANCE hInstance, const wchar_t* title, int width, int height)
{
	WNDCLASSEXW wc = {};

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = title;
	RegisterClassExW(&wc);

	HWND hwnd = CreateWindowExW(
		0,
		title,
		L"Daylight Engine",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1280, 720,
		nullptr, nullptr, hInstance, nullptr);

	ShowWindow(hwnd, SW_SHOW);
}

Window::~Window()
{
	if (hWnd)
	{
		DestroyWindow(hWnd);
	}
}

bool Window::ProcessMessages()
{
	MSG msg = {};

	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
		{
			return false;
		}
	}

	return true;
}
