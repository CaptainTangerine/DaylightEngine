#include "pch.h"
#include "EditorApplication.h"
#include "Runtime/Core/Engine.h"
#include <stdexcept>

#include <SDL3/SDL.h>
#include <iomanip>

#include <Runtime/Core/Paths.h>
#include <Runtime/Utils/Image.h>

namespace Dlight
{
	void EditorApplication::ShowError(const std::string& message) const
	{
		DL_LOG_ERROR(message);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Daylight - Error", message.c_str(), window);
	}

	EditorApplication::EditorApplication()
	{
		try
		{
			Initialize();
		}
		catch (...)
		{
			Shutdown();
			throw;
		}
	}

	EditorApplication::~EditorApplication()
	{
		Shutdown();
	}

	void EditorApplication::Initialize()
	{
		if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
		{
			const std::string message = SDL_GetError();
			ShowError(message);
			throw std::runtime_error(message);
		}

		window = SDL_CreateWindow(
			"Daylight",
			static_cast<int>(width),
			static_cast<int>(height),
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		if (!window)
		{
			const std::string message = SDL_GetError();
			ShowError(message);
			throw std::runtime_error(message);
		}

		SetWindowIcon();
		DL_LOG_INFO("Window created: ", width, "x", height);

		int pixelWidth = 0;
		int pixelHeight = 0;
		if (!SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight))
		{
			const std::string message = SDL_GetError();
			ShowError(message);
			throw std::runtime_error(message);
		}

		width = static_cast<uint32>(pixelWidth);
		height = static_cast<uint32>(pixelHeight);
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

			if (!engine->PrepareFrame(width, height))
			{
				SDL_Delay(10);
				ResetFrameStats();
				continue;
			}
			if (engine->AcquireNextImage())
			{
				UpdateDeltaTime();
				engine->RenderFrame();
				UpdateFrameStats();
			}
			else
			{
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
		// Engine이 surface/device까지 해제한 뒤 SDL 창을 파괴한다.
		engine.reset();

		if (window)
		{
			SDL_DestroyWindow(window);
			window = nullptr;
		}

		// SDL 초기화 실패 시에도 호출할 수 있다.
		SDL_Quit();
	}
}
