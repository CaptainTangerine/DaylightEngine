#pragma once

#include "vulkan/vulkan.h"
#include <vma/vk_mem_alloc.h>

namespace Dlight
{
	class VulkanDevice;

	// Device Local image 생성
	class VulkanImage
	{
	public:
		VulkanImage(
			VulkanDevice& _device,
			const VkImageCreateInfo& imageCreateInfo,
			const VkImageViewCreateInfo& imageViewCreateInfo);
		~VulkanImage();

	public:
		VkImage GetImage() const { return image; };
		VkImageView GetImageView() const { return imageView; };
		VkImageUsageFlags GetUsage() const { return usage; };
		VkFormat GetFormat() const { return format; };
		uint32 GetWidth() const { return width; };
		uint32 GetHeight() const { return height; };

	private:
		VulkanDevice& device;

		VkImage	image = { VK_NULL_HANDLE };
		VkImageView imageView = { VK_NULL_HANDLE };
		VmaAllocation allocation = { VK_NULL_HANDLE };

		VkFormat format = {};
		uint32 width = {};
		uint32 height = {};
		VkImageUsageFlags usage = {};
	};
}
