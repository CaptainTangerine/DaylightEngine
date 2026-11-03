#include "pch.h"
#include "EditorApplication.h"

#include <SDL3/SDL.h>
#include <iomanip>

#include <Core/Paths.h>
#include <Runtime/Core/Engine.h>
#include <Utils/Image.h>

namespace Dlight
{
    EditorApplication::EditorApplication()
    {
        Initialize();
    }

    EditorApplication::~EditorApplication()
    {
        Shutdown();
    }

    void EditorApplication::ShowError(const std::string& message) const
    {
        DL_LOG_ERROR(message);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Daylight - Error", message.c_str(), window);
    }

    void EditorApplication::Initialize()
    {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            ShowError(SDL_GetError());
            std::abort();
        }

        window = SDL_CreateWindow(
            "Daylight",
            static_cast<int>(width),
            static_cast<int>(height),
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        if (!window)
        {
            ShowError(SDL_GetError());
            std::abort();
        }

        SetWindowIcon();
        DL_LOG_INFO("Window created: ", width, "x", height);

        int pixelWidth = 0;
        int pixelHeight = 0;
        if (!SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight))
        {
            ShowError(SDL_GetError());
            std::abort();
        }

        width = static_cast<uint32>(pixelWidth);
        height = static_cast<uint32>(pixelHeight);

        // Engine은 SDL_Window*를 빌려 Vulkan Surface를 만들며 창 자체는 소유하지 않는다.
        engine = std::make_unique<Engine>(window, width, height);
    }

    void EditorApplication::SetWindowIcon()
    {
        const std::filesystem::path iconPath = Paths::GetAssetPath(L"Image/Icon.png");
        Image iconImage(iconPath.string());

        SDL_Surface* iconSurface = SDL_CreateSurfaceFrom(
            iconImage.GetWidth(),
            iconImage.GetHeight(),
            SDL_PIXELFORMAT_RGBA32,
            iconImage.GetData(),
            iconImage.GetWidth() * 4);

        if (!iconSurface)
        {
            DL_LOG_WARN("Failed to create window icon surface: ", SDL_GetError());
            return;
        }

        if (!SDL_SetWindowIcon(window, iconSurface))
        {
            DL_LOG_WARN("Failed to set window icon: ", SDL_GetError());
        }

        SDL_DestroySurface(iconSurface);
    }

    void EditorApplication::Run()
    {
        bRunning = true;
        ResetFrameStats();

        while (bRunning)
        {
            SDL_Event event{};
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT ||
                    event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                {
                    bRunning = false;
                    break;
                }
                if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                {
                    width = static_cast<uint32>(event.window.data1);
                    height = static_cast<uint32>(event.window.data2);
                }
            }

            if (!bRunning)
            {
                break;
            }

            if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) || width == 0 || height == 0)
            {
                SDL_Delay(10);
                ResetFrameStats();
                continue;
            }

            if (engine->RenderFrame(width, height))
            {
                UpdateDeltaTime();
                UpdateFrameStats();
            }
            else
            {
                SDL_Delay(10);
                ResetFrameStats();
            }
        }
    }

    void EditorApplication::ResetFrameStats()
    {
        const auto now = FrameClock::now();
        previousFrameTime = now;
        logSampleStart = now;
        deltaTime = 0.0f;
    }

    void EditorApplication::UpdateDeltaTime()
    {
        const auto now = FrameClock::now();
        deltaTime = std::chrono::duration<float>(now - previousFrameTime).count();
        previousFrameTime = now;
    }

    void EditorApplication::UpdateFrameStats()
    {
        if (deltaTime <= 0.0f)
        {
            return;
        }

        const auto now = FrameClock::now();
        if (now - logSampleStart >= std::chrono::seconds(1))
        {
            const float fps = 1.0f / deltaTime;
            const float frameTime = deltaTime * 1000.0f;

            std::ostringstream title;
            title << std::fixed << std::setprecision(2)
                  << "Daylight | " << fps << " FPS | "
                  << frameTime << " ms";

            const std::string titleText = title.str();
            SDL_SetWindowTitle(window, titleText.c_str());
            DL_LOG_INFO(titleText);
            logSampleStart = now;
        }
    }

    void EditorApplication::Shutdown()
    {
        // Vulkan Surface/Device는 SDL 창보다 먼저 파괴되어야 한다.
        engine.reset();

        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        SDL_Quit();
    }
}
