#pragma once

#include <cstdint>
#include <memory>

#include "CoreTypes.h"

struct SDL_Window;

namespace Dlight
{
    class VulkanDevice;
    class Renderer;
    class ResourceManager;

    // 공통 런타임: SDL 창을 소유하지 않고 전달받아 Vulkan 렌더러를 구성한다.
    class Engine
    {
    public:
        Engine(SDL_Window* window, uint32 width, uint32 height);
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // 스왑체인 재생성이 보류되거나 이미지를 획득하지 못하면 false.
        bool RenderFrame(uint32 width, uint32 height);

    private:
        void Initialize(SDL_Window* window, uint32 width, uint32 height);
        void Shutdown();

        std::unique_ptr<VulkanDevice> vulkanDevice;
        std::unique_ptr<ResourceManager> resourceManager;
        std::unique_ptr<Renderer> renderer;
    };
}
