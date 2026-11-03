#include "pch.h"
#include "Engine.h"

#include <Graphics/Vulkan/VulkanDevice.h>
#include <Graphics/Vulkan/VulkanSwapchain.h>
#include <Rendering/Renderer.h>
#include <Resources/ResourceManager.h>

namespace Dlight
{
    Engine::Engine(SDL_Window* window, uint32 width, uint32 height)
    {
        Initialize(window, width, height);
    }

    Engine::~Engine()
    {
        Shutdown();
    }

    void Engine::Initialize(SDL_Window* window, uint32 width, uint32 height)
    {
        vulkanDevice = std::make_unique<VulkanDevice>(window, width, height);
        resourceManager = std::make_unique<ResourceManager>();
        renderer = std::make_unique<Renderer>(*vulkanDevice, *resourceManager);
    }

    bool Engine::RenderFrame(uint32 width, uint32 height)
    {
        bool swapchainRecreated = false;
        if (!vulkanDevice->UpdateSwapchain(width, height, swapchainRecreated))
        {
            return false;
        }

        if (swapchainRecreated)
        {
            renderer->OnResize(
                vulkanDevice->GetSwapchain().GetWidth(),
                vulkanDevice->GetSwapchain().GetHeight());
        }

        if (!vulkanDevice->AcquireNextImage())
        {
            return false;
        }

        vulkanDevice->BeginFrame();
        renderer->Render();
        vulkanDevice->EndFrame();
        return true;
    }

    void Engine::Shutdown()
    {
        if (vulkanDevice && vulkanDevice->GetDevice() != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(vulkanDevice->GetDevice());
        }

        renderer.reset();
        resourceManager.reset();
        vulkanDevice.reset();
    }
}
