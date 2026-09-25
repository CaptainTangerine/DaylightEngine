#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class VulkanDevice;

	class VulkanSwapchain
	{
	public:
		explicit VulkanSwapchain(VulkanDevice& device);
		~VulkanSwapchain();

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

		bool Initialize(uint32_t width, uint32_t height);
		void Shutdown();

	private:
		VulkanDevice& device;
		VkSwapchainKHR swapchain = { VK_NULL_HANDLE };
		uint32_t swapchainWidth = 0;
		uint32_t swapchainHeight = 0;
	};
}
