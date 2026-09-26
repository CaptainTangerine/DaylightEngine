#include "pch.h"
#include "Engine.h"

#include <SDL3/SDL.h>

#include <Graphics/Vulkan/VulkanDevice.h>
#include <Rendering/Renderer.h>
#include <cstdlib>

namespace Dlight
{
	void Engine::ShowError(const std::string& message) const
	{
		DL_LOG_ERROR(message);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Daylight - Error", message.c_str(), window);
	}

	Engine::~Engine()
	{
		Shutdown();
	}

	Engine::Engine()
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
	}

	void Engine::Run()
	{
		bRunning = true;
		while (bRunning)
		{
			SDL_Event event{ 0 };

			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_QUIT)
				{
					bRunning = false;
					break;
				}
				else if (event.type == SDL_EVENT_WINDOW_RESIZED)
				{
					width = event.window.data1;
					height = event.window.data2;
					break;
				}

			}
		}
	}

	void Engine::Shutdown()
	{
		renderer.reset();
		vulkanDevice.reset();

		if (window)
		{
			SDL_DestroyWindow(window);
			window = nullptr;
		}

		SDL_Quit();
	}

}
