#include "pch.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"

Dlight::VulkanImage::VulkanImage(
	VulkanDevice& _device,
	const FVulkanImageDesc& desc)
	: device(_device)
	, format(desc.format)
	, width(desc.extent.width)
	, height(desc.extent.height)
	, usage(desc.usage)
{
	if (desc.extent.width == 0 || desc.extent.height == 0 ||
		desc.extent.depth == 0 ||
		desc.format == VK_FORMAT_UNDEFINED ||
		desc.mipLevels == 0 || desc.arrayLayers == 0 ||
		desc.usage == 0 || desc.aspectMask == 0)
	{
		DL_LOG_ERROR("Invalid image description");
		std::abort();
	}

	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.flags = desc.flags;
	imageCreateInfo.imageType = desc.imageType;
	imageCreateInfo.format = desc.format;
	imageCreateInfo.extent = desc.extent;
	imageCreateInfo.mipLevels = desc.mipLevels;
	imageCreateInfo.arrayLayers = desc.arrayLayers;
	imageCreateInfo.samples = desc.samples;
	imageCreateInfo.tiling = desc.tiling;
	imageCreateInfo.usage = desc.usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.flags = desc.allocationFlags;
	allocInfo.usage = desc.memoryUsage;

	if (VK_SUCCESS != vmaCreateImage(device.GetVmaAllocator(), &imageCreateInfo, &allocInfo, &image, &allocation, nullptr))
	{
		DL_LOG_ERROR("Failed to create image");
		std::abort();
	}

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = desc.viewType;
	viewInfo.format = desc.format;
	viewInfo.subresourceRange.aspectMask = desc.aspectMask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = desc.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = desc.arrayLayers;

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
