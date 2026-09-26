#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;

	constexpr static VkFormat depthStencilFormat{ VK_FORMAT_D32_SFLOAT_S8_UINT };

	class Renderer
	{
	public:
		Renderer(VulkanDevice& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		bool CreateDepthStencilResources();
		void DestroyDepthStencilResources();
			
	private:
		VulkanDevice& device;
		
		// Depth-stencil buffer
		VkImage depthStencilImage = { nullptr };
		VkImageView depthStencilImageView = { nullptr };
		VmaAllocation depthStencilImageAllocation = { nullptr };


	};
}
