#include "pch.h"
#include "Engine.h"

#include <SDL3/SDL.h>
#include <iomanip>

#include <Core/Paths.h>
#include <Graphics/Vulkan/VulkanDevice.h>
#include <Graphics/Vulkan/VulkanSwapchain.h>
#include <Rendering/Renderer.h>
#include <Resources/ResourceManager.h>
#include <Utils/Image.h>

namespace Dlight
{
	void Engine::ShowError(const std::string& message) const
	{
		DL_LOG_ERROR(message);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Daylight - Error", message.c_str(), window);
	}

	Engine::Engine()
	{
		Initialize();
	}

	Engine::~Engine()
	{
		Shutdown();
	}

	void Engine::Initialize()
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

		vulkanDevice = std::make_unique<VulkanDevice>(
			window, static_cast<uint32>(pixelWidth), static_cast<uint32>(pixelHeight));
		width = static_cast<uint32>(pixelWidth);
		height = static_cast<uint32>(pixelHeight);

		resourceManager = std::make_unique<ResourceManager>();
		renderer = std::make_unique<Renderer>(*vulkanDevice, *resourceManager);
	}

	void Engine::SetWindowIcon()
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

	void Engine::Run()
	{
		bRunning = true;
		ResetFrameStats();

		while (true)
		{
			SDL_Event event{ 0 };

			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_QUIT ||
					event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
				{
					bRunning = false;
					break;
				}
				else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
				{
					width = event.window.data1;
					height = event.window.data2;
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

			bool swapchainRecreated = false;
			if (!vulkanDevice->UpdateSwapchain(width, height, swapchainRecreated))
			{
				SDL_Delay(10);
				ResetFrameStats();
				continue;
			}

			if (swapchainRecreated)
			{
				renderer->OnResize(
					vulkanDevice->GetSwapchain().GetWidth(),
					vulkanDevice->GetSwapchain().GetHeight());
			}

			if (vulkanDevice->AcquireNextImage())
			{
				UpdateDeltaTime();
				vulkanDevice->BeginFrame();
				renderer->Render();
				vulkanDevice->EndFrame();
				UpdateFrameStats();
			}
			else
			{
				ResetFrameStats();
			}
		}
	}

	void Engine::ResetFrameStats()
	{
		const auto now = FrameClock::now();
		previousFrameTime = now;
		logSampleStart = now;
		deltaTime = 0.0f;
	}

	void Engine::UpdateDeltaTime()
	{
		const auto now = FrameClock::now();
		deltaTime = std::chrono::duration<float>(now - previousFrameTime).count();
		previousFrameTime = now;
	}

	void Engine::UpdateFrameStats()
	{
		if (deltaTime <= 0.0f)
		{
			return;
		}

		const float fps = 1.0f / deltaTime;

		std::ostringstream title;
		title << std::fixed << std::setprecision(2)
			<< "Daylight | " << fps << " FPS | "
			<< deltaTime * 1000.0f << " ms dt";

		const std::string titleText = title.str();
		SDL_SetWindowTitle(window, titleText.c_str());

		const auto now = FrameClock::now();
		if (now - logSampleStart >= std::chrono::seconds(1))
		{
			DL_LOG_INFO(titleText);
			logSampleStart = now;
		}
	}

	void Engine::Shutdown()
	{
		vkDeviceWaitIdle(vulkanDevice->GetDevice());

		renderer.reset();
		resourceManager.reset();
		vulkanDevice.reset();

		if (window)
		{
			SDL_DestroyWindow(window);
			window = nullptr;
		}

		SDL_Quit();
	}

}
