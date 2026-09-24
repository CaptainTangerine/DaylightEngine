#pragma once

#include "Graphics/Vulkan/VulkanDevice.h"

struct SDL_Window;

namespace Dlight
{
	class Engine
	{
	public:
		Engine() = default;
		~Engine() = default;

	public:
		bool Initialize();
		void Run();
		void Shutdown();

	private:
		void ShowError(const std::string& message) const;

	private:
		// Window
		SDL_Window* window = { nullptr } ;
		uint32 width = { 1920 };
		uint32 height = { 1080 };
		bool	 bRunning = { false };

		VulkanDevice vulkanDevice;
	};
}
