#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class VulkanDevice;

	class VulkanSwapchain
	{
	public:
		constexpr static VkFormat swapchainFormat = { VK_FORMAT_B8G8R8A8_SRGB };

	public:
		VulkanSwapchain(VulkanDevice& device, uint32 width, uint32 height);
		~VulkanSwapchain();

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

	public:
		VkSwapchainKHR GetSwapchain() { return swapchain; };
		VkImage		   GetSwapchianImage(uint32 index)  const { return swapchainImages[index]; };
		VkImageView GetSwapchainImageView(uint32 index) const { return swapchainImageViews[index]; }
		VkSemaphore GetRendercompleteSemaphore(uint32 index) const { return renderCompleteSemaphores[index]; };

		uint32 GetWidth() const { return swapchainWidth; }
		uint32 GetHeight() const { return swapchainHeight; }

	public:
		// 크기가 0이면 기존 자원을 유지하고 생성을 보류한다.
		bool Initialize(uint32 width, uint32 height);
		void Shutdown();

	private:
		VulkanDevice& device;
		VkSwapchainKHR swapchain = { VK_NULL_HANDLE };
		
		uint32 swapchainWidth = { 0 };
		uint32 swapchainHeight = { 0 };

		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImageViews;
		std::vector<VkSemaphore> renderCompleteSemaphores;

	};
}
