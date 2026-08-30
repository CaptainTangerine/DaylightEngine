#include "pch.h"
#include "Platform/Window.h"

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    Window  window(hInstance, L"Daylight Engine", 1280, 720);

    while (window.ProcessMessages())
    {

    }

    return 0;
}

