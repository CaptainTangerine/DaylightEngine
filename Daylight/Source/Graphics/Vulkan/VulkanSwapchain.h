#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class VulkanDevice;

	class VulkanSwapchain
	{
		constexpr static VkFormat swapchainFormat = { VK_FORMAT_B8G8R8A8_SRGB };

	public:
		explicit VulkanSwapchain(VulkanDevice& device);
		~VulkanSwapchain();

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

	public:
		VkSwapchainKHR GetSwapchain() { return swapchain; };

	public:
		bool Initialize(uint32 width, uint32 height);
		void Shutdown();

	private:
		VulkanDevice& device;
		VkSwapchainKHR swapchain = { VK_NULL_HANDLE };
		
		bool bReqireSwapchainRecreate = false;
		uint32 swapchainWidth = 0;
		uint32 swapchainHeight = 0;

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		std::vector<VkSemaphore> renderCompleteSemaphores;

	};
}
