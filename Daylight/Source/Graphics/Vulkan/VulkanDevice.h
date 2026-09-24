#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;

namespace Dlight
{
	class VulkanDevice
	{
	public:
		bool Initialize(SDL_Window* window);
		void Shutdown();

		VkInstance GetInstance() const { return vulkanInstance; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkDevice GetDevice() const { return device; }
		VkQueue GetGraphicsQueue() const { return gfxQueue; }
		uint32_t GetGraphicsQueueFamilyIndex() const { return gfxQueueFamilyIndex; }
		VkSurfaceKHR GetSurface() const { return vulkanSurface; }

	private:
		bool InitializeVulkan();
		bool InitializeSurface(SDL_Window* window);
		VkPhysicalDevice FindPhysicalDevice() const;
		bool FindGraphicsQueue();
		bool CreateDevice();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		// Vulkan Core
		VkInstance vulkanInstance = { VK_NULL_HANDLE };
		VkSurfaceKHR vulkanSurface = { VK_NULL_HANDLE };
		VkPhysicalDevice physicalDevice = { VK_NULL_HANDLE };
		VkDevice device = { VK_NULL_HANDLE };

		// Queue Related
		uint32 gfxQueueFamilyIndex = UINT32_MAX;
		VkQueue gfxQueue = VK_NULL_HANDLE;

		bool volkInitialized = false;
	};
}
