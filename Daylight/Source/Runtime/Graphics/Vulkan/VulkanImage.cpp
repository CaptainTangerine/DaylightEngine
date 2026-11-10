#include "pch.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"

std::unique_ptr<Dlight::VulkanImage> Dlight::VulkanImage::whiteDefaultImage;
std::unique_ptr<Dlight::VulkanImage> Dlight::VulkanImage::blackDefaultImage;
std::unique_ptr<Dlight::VulkanImage> Dlight::VulkanImage::flatNormalDefaultImage;

void Dlight::VulkanImage::InitializeDefaultImage(VulkanDevice& device)
{
	if (whiteDefaultImage && blackDefaultImage && flatNormalDefaultImage)
	{
		return;
	}

	constexpr std::array<uint8, 4> WhitePixel = { 255, 255, 255, 255 };
	constexpr std::array<uint8, 4> BlackPixel = { 0, 0, 0, 255 };
	constexpr std::array<uint8, 4> FlatNormalPixel = { 128, 128, 255, 255 };

	FVulkanImageDesc imageDesc{};
	imageDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageDesc.extent = { 1, 1, 1 };
	imageDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	whiteDefaultImage = std::make_unique<VulkanImage>(device, imageDesc);
	blackDefaultImage = std::make_unique<VulkanImage>(device, imageDesc);
	flatNormalDefaultImage = std::make_unique<VulkanImage>(device, imageDesc);

	device.UploadImage(*whiteDefaultImage, WhitePixel.data(), WhitePixel.size(), EImageState::FragmentSampledRead);
	device.UploadImage(*blackDefaultImage, BlackPixel.data(), BlackPixel.size(), EImageState::FragmentSampledRead);
	device.UploadImage(*flatNormalDefaultImage, FlatNormalPixel.data(), FlatNormalPixel.size(), EImageState::FragmentSampledRead);
}

void Dlight::VulkanImage::DestroyDefaultImage()
{
	whiteDefaultImage.reset();
	blackDefaultImage.reset();
	flatNormalDefaultImage.reset();
}

const Dlight::VulkanImage& Dlight::VulkanImage::GetDefaultImage(EDefaultImage type)
{
	const VulkanImage* defaultImage = nullptr;
	switch (type)
	{
	case EDefaultImage::White:
		defaultImage = whiteDefaultImage.get();
		break;
	case EDefaultImage::Black:
		defaultImage = blackDefaultImage.get();
		break;
	case EDefaultImage::FlatNormal:
		defaultImage = flatNormalDefaultImage.get();
		break;
	}

	if (!defaultImage)
	{
		DL_LOG_ERROR("Default Vulkan image has not been initialized");
		std::abort();
	}

	return *defaultImage;
}

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
