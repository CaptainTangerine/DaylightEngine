#include "pch.h"
#include "Platform/Window.h"

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    Window window(hInstance, L"Daylight Engine", 1280, 720);

    DelegateHandle resizeHandle = window.onResize.Add([](int width, int height) {});

    while (window.ProcessMessages())
    {
        // 렌더링 로직
    }

    window.onResize.Remove(resizeHandle);

    return 0;
}

