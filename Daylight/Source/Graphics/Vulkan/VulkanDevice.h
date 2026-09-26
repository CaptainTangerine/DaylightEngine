#pragma once

#include <vulkan/vulkan.h>
#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

struct SDL_Window;

namespace Dlight
{
	class VulkanSwapchain;

	class VulkanDevice
	{
		constexpr static uint32   vulkanVersion = { VK_API_VERSION_1_4 };

	public:
		VulkanDevice(SDL_Window* window, uint32 width, uint32 height);
		~VulkanDevice();

	public:
		VkInstance GetInstance() const { return vulkanInstance; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkDevice GetDevice() const { return device; }
		VkQueue GetGraphicsQueue() const { return gfxQueue; }
		uint32 GetGraphicsQueueFamilyIndex() const { return gfxQueueFamilyIndex; }
		VkSurfaceKHR GetSurface() const { return vulkanSurface; }
		VmaAllocator GetVmaAllocator() const { return vmaAllocator;  }
		VulkanSwapchain& GetSwapchain() { return *swapchain; }
		const VulkanSwapchain& GetSwapchain() const { return *swapchain; }

	private:
		void Shutdown();

		bool InitializeVulkan();
		bool InitializeSurface(SDL_Window* window);
		VkPhysicalDevice FindPhysicalDevice() const;
		bool FindGraphicsQueue();
		bool CreateDevice();
		bool InitializeVMA();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		bool volkInitialized = { false };

		// Vulkan Core
		VkInstance vulkanInstance = { VK_NULL_HANDLE };
		VkSurfaceKHR vulkanSurface = { VK_NULL_HANDLE };
		VkPhysicalDevice physicalDevice = { VK_NULL_HANDLE };
		VkDevice device = { VK_NULL_HANDLE };

		// Queue Related
		uint32 gfxQueueFamilyIndex = { UINT32_MAX } ;
		VkQueue gfxQueue = { VK_NULL_HANDLE };

		// Vulkan Memory Allocater
		VmaAllocator vmaAllocator = { nullptr };

		// Swapchain
		std::unique_ptr<VulkanSwapchain> swapchain;

	};
}
