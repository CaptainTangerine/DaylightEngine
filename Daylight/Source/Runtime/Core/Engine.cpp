#include "pch.h"
#include "Engine.h"

#include <stdexcept>
#include <Graphics/Vulkan/VulkanDevice.h>
#include <Graphics/Vulkan/VulkanSwapchain.h>
#include <Rendering/Renderer.h>
#include <Resources/ResourceManager.h>

namespace Dlight
{
	Engine::Engine(SDL_Window* window, uint32 width, uint32 height)
	{
		try
		{
			Initialize(window, width, height);
		}
		catch (...)
		{
			Shutdown();
			throw;
		}
	}

	Engine::~Engine()
	{
		Shutdown();
	}

	void Engine::Initialize(SDL_Window* window, uint32 width, uint32 height)
	{
		if (!window)
		{
			throw std::invalid_argument("Engine requires an SDL window");
		}

		vulkanDevice = std::make_unique<VulkanDevice>(window, width, height);
		resourceManager = std::make_unique<ResourceManager>();
		renderer = std::make_unique<Renderer>(*vulkanDevice, *resourceManager);
	}

	bool Engine::PrepareFrame(uint32 width, uint32 height)
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

		return true;
	}

	bool Engine::AcquireNextImage()
	{
		return vulkanDevice->AcquireNextImage();
	}

	void Engine::RenderFrame(const FViewData& viewData)
	{
		vulkanDevice->BeginFrame();
		renderer->Render(viewData);
		vulkanDevice->EndFrame();
	}

	void Engine::Shutdown()
	{
		if (vulkanDevice && vulkanDevice->GetDevice())
		{
			vkDeviceWaitIdle(vulkanDevice->GetDevice());
		}

		renderer.reset();
		resourceManager.reset();
		vulkanDevice.reset();
	}
}
