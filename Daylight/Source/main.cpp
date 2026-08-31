#include "pch.h"
#include "Platform/Window.h"
#include "Graphics/GfxDevice.h"
#include "Graphics/GfxShader.h"
#include "Renderer/Renderer.h"

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    Window window(hInstance, L"Daylight Engine", 1280, 720);

    GfxDevice gfx;
    GfxDeviceDesc desc;
    desc.windowHandle = window.GetHandle();
    desc.width  = 1280;
    desc.height = 720;

#if defined(_DEBUG)
    desc.debugLayer = true;
#endif

    if (!gfx.Init(desc))
    {
        return -1;
    }
    Renderer  renderer;
    if (!renderer.Init(gfx))
    {
        return -1;
    }

    DelegateHandle resizeHandle = window.onResize.Add(
        [&gfx](int w, int h)
        {
            gfx.Resize(w, h);
        }
    );

    while (window.ProcessMessages())
    {
        gfx.BeginFrame();
        renderer.Render(gfx);
        gfx.EndFrame(true);
    }

    window.onResize.Remove(resizeHandle);
    gfx.Shutdown();

    return 0;
}
