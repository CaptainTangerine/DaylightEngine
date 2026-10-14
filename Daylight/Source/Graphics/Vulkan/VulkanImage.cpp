#include "pch.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"

Dlight::VulkanImage::VulkanImage(VulkanDevice& _device, const FVulkanImageDesc& desc)
	: device(_device)
	, width(desc.width)
	, height(desc.height)
	, usage(desc.usage)
{
	if (desc.width == 0 || desc.height == 0 ||
		desc.format == VK_FORMAT_UNDEFINED || desc.usage == 0)
	{
		DL_LOG_ERROR("Invalid image description");
		std::abort();
	}

	// GPU 이미지와 메모리 생성
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = desc.format;
	imageInfo.extent = { desc.width, desc.height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = desc.usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = desc.memoryUsage;
	allocInfo.flags = desc.allocationFlags;

	
	if (VK_SUCCESS != vmaCreateImage(device.GetVmaAllocator(), &imageInfo, &allocInfo, &image, &allocation, nullptr))
	{
		DL_LOG_ERROR("Failed to create image");
		std::abort();
	}

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = desc.format;

	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (VK_SUCCESS != vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &imageView))
	{
		vmaDestroyImage(device.GetVmaAllocator(), image, allocation);
		image = VK_NULL_HANDLE;
		allocation = VK_NULL_HANDLE;

		DL_LOG_ERROR("Failed to create image view");
		std::abort();
	}
}

Dlight::VulkanImage::~VulkanImage()
{
	if (VK_NULL_HANDLE != imageView)
	{
		vkDestroyImageView(device.GetDevice(), imageView, nullptr);
	}

	if (VK_NULL_HANDLE != image)
	{
		vmaDestroyImage(device.GetVmaAllocator(), image, allocation);
	}
}
