#include "pch.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"

Dlight::VulkanImage::VulkanImage(
	VulkanDevice& _device,
	const VkImageCreateInfo& imageCreateInfo,
	const VkImageViewCreateInfo& imageViewCreateInfo)
	: device(_device)
	, width(imageCreateInfo.extent.width)
	, height(imageCreateInfo.extent.height)
	, usage(imageCreateInfo.usage)
{
	if (imageCreateInfo.sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO ||
		imageViewCreateInfo.sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO ||
		imageCreateInfo.extent.width == 0 || imageCreateInfo.extent.height == 0 ||
		imageCreateInfo.extent.depth == 0 ||
		imageCreateInfo.format == VK_FORMAT_UNDEFINED ||
		imageViewCreateInfo.format == VK_FORMAT_UNDEFINED ||
		imageCreateInfo.usage == 0)
	{
		DL_LOG_ERROR("Invalid image description");
		std::abort();
	}

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	if (VK_SUCCESS != vmaCreateImage(device.GetVmaAllocator(), &imageCreateInfo, &allocInfo, &image, &allocation, nullptr))
	{
		DL_LOG_ERROR("Failed to create image");
		std::abort();
	}

	VkImageViewCreateInfo viewInfo = imageViewCreateInfo;
	viewInfo.image = image;

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
