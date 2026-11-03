#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "CoreTypes.h"

struct SDL_Window;

namespace Dlight
{
    class Engine;

    // SDL 창과 이벤트 루프를 소유하는 Daylight 실행 진입 계층.
    class EditorApplication
    {
    public:
        EditorApplication();
        ~EditorApplication();

        EditorApplication(const EditorApplication&) = delete;
        EditorApplication& operator=(const EditorApplication&) = delete;

        void Run();

    private:
        void Initialize();
        void Shutdown();
        void ShowError(const std::string& message) const;
        void SetWindowIcon();
        void ResetFrameStats();
        void UpdateDeltaTime();
        void UpdateFrameStats();

        SDL_Window* window = nullptr;
        uint32 width = 1280;
        uint32 height = 720;
        bool bRunning = false;

        using FrameClock = std::chrono::steady_clock;
        FrameClock::time_point previousFrameTime{};
        FrameClock::time_point logSampleStart{};
        float deltaTime = 0.0f;

        std::unique_ptr<Engine> engine;
    };
}
