#pragma once

#include "CoreTypes.h"
#include "vulkan/vulkan.h"
#include <vma/vk_mem_alloc.h>

namespace Dlight
{
	class VulkanDevice;

	struct FVulkanImageDesc
	{
		VkImageType imageType = VK_IMAGE_TYPE_2D;
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
		VkExtent3D extent = { 0, 0, 1 };
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32 mipLevels = 1;
		uint32 arrayLayers = 1;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags usage = 0;
		VkImageCreateFlags flags = 0;
		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		VmaAllocationCreateFlags allocationFlags = 0;
	};

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
