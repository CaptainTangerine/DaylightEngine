#include "pch.h"
#include "Engine.h"

#include <SDL3/SDL.h>

namespace Dlight
{
	void Engine::ShowError(const std::string& message) const
	{
		DL_LOG_ERROR(message);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Daylight - Error", message.c_str(), window);
	}

	bool Engine::Initialize()
	{
		if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
		{
			ShowError(SDL_GetError());
			return false;
		}

		window = SDL_CreateWindow(
			"Daylight",
			static_cast<int>(width),
			static_cast<int>(height),
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		if (!window)
		{
			ShowError(SDL_GetError());
			return false;
		}

		DL_LOG_INFO("Window created: ", width, "x", height);

		if (!vulkanDevice.Initialize(window))
		{
			ShowError("Can't initialize Vulkan device");
			return false;
		}
		return true;
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
		vulkanDevice.Shutdown();

		if (window)
		{
			SDL_DestroyWindow(window);
			window = nullptr;
		}

		SDL_Quit();
	}

}
