#pragma once

#include "vulkan/vulkan.h"
#include <vma/vk_mem_alloc.h>

namespace Dlight
{
	struct FVulkanImageDesc
	{
		uint32 width = 0;
		uint32 height = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;

		VkImageUsageFlags usage = 0;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
		VmaAllocationCreateFlags allocationFlags = 0;
	};

	class VulkanDevice;

	// Device Local image 생성
	class VulkanImage
	{
	public:
		VulkanImage(VulkanDevice& _device, const FVulkanImageDesc& desc);
		~VulkanImage();

	public:
		VkImage GetImage() const { return image; };
		VkImageView GetImageView() const { return imageView; };
		VkImageUsageFlags GetUsage() const { return usage; };
		uint32 GetWidth() const { return width; };
		uint32 GetHeight() const { return height; };

	private:
		VulkanDevice& device;

		VkImage	image = { VK_NULL_HANDLE };
		VkImageView imageView = { VK_NULL_HANDLE };
		VmaAllocation allocation = { VK_NULL_HANDLE };

		uint32 width = {};
		uint32 height = {};
		VkImageUsageFlags usage = {};

		void* mappedData = {};

	};
}
