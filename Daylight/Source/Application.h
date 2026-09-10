#pragma once

#include "CoreTypes.h"

#include <string>
#include <vulkan/vulkan.h>

struct SDL_Window;

namespace Dlight
{
	class Application
	{
	public:
		Application() = default;
		~Application() = default;

	public:
		bool Initialize();
		void Run();
		void Shutdown();

	private:
		bool InitializeVulkan();
		void ShowError(const std::string& message) const;


	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		SDL_Window* window = { nullptr } ;
		uint32 width = { 1280 };
		uint32 height = { 720 };
		bool	 bRunning = { false };


		// Vulkan Core
		VkInstance vulkanInstance = { nullptr };

	};
}
