#include "pch.h"
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include <cstdlib>

namespace Dlight
{
	VulkanSwapchain::~VulkanSwapchain()
	{
		Shutdown();
	}

	VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, uint32 width, uint32 height)
		: device(device)
	{
		swapchainWidth  = width;
		swapchainHeight = height;

		// 지원되는 포맷인지 검사
		uint32 formatCount = 0;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), device.GetSurface(), &formatCount, nullptr) != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to get surface format count");
			std::abort();
		}
		if (formatCount == 0)
		{
			DL_LOG_ERROR("No surface formats are available");
			std::abort();
		}
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), device.GetSurface(), &formatCount, surfaceFormats.data()) != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to get surface formats");
			std::abort();
		}
		surfaceFormats.resize(formatCount);

		bool formatSupported = false;
		for (const VkSurfaceFormatKHR& elem : surfaceFormats)
		{
			if (elem.format == swapchainFormat && elem.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				formatSupported = true;
				break;
			}
		}

		if (!formatSupported)
		{
			DL_LOG_ERROR("Requested swapchain format is not supported by the surface");
			std::abort();
		}


		VkSurfaceCapabilitiesKHR surfaceCaps{};
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetPhysicalDevice(), device.GetSurface(), &surfaceCaps))
		{
			DL_LOG_ERROR("Can't get the surface capabilities");
			std::abort();
		}

		// 스왑체인 생성
		VkSwapchainCreateInfoKHR swapchainCreateInfo{};

		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = device.GetSurface();
		swapchainCreateInfo.minImageCount = surfaceCaps.minImageCount;
		swapchainCreateInfo.imageFormat = swapchainFormat;
		swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		swapchainCreateInfo.imageExtent.width = swapchainWidth;
		swapchainCreateInfo.imageExtent.height = swapchainHeight;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		
		if (vkCreateSwapchainKHR(device.GetDevice(), &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
		{
			DL_LOG_ERROR("Error creating swapchain");
			std::abort();
		}

		// 실제 스왑체인이 관리하는 이미지 개수 가져오기
		uint32 imageCount = 0;
		vkGetSwapchainImagesKHR(device.GetDevice(), swapchain, &imageCount, nullptr);
		swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device.GetDevice(), swapchain, &imageCount, swapchainImages.data());

		swapchainImageViews.resize(imageCount);
		
		for (size_t i = 0; i < imageCount; ++i)
		{
			VkImageViewCreateInfo imgViewInfo = {};
			imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imgViewInfo.image = swapchainImages[i];
			imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imgViewInfo.format = swapchainFormat;
			imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgViewInfo.subresourceRange.levelCount = 1;
			imgViewInfo.subresourceRange.layerCount = 1;

			if (VK_SUCCESS != vkCreateImageView(device.GetDevice(), &imgViewInfo, nullptr, &swapchainImageViews[i]))
			{
				DL_LOG_ERROR("Error Createing Swapchian ImageViews");
				std::abort();
			}
		}

		renderCompleteSemaphores.resize(imageCount);

		for (VkSemaphore& semaphore : renderCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (VK_SUCCESS != vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &semaphore))
			{
				DL_LOG_ERROR("Error create render complete semaphore");
				std::abort();
			}
		}
		
	}

	void VulkanSwapchain::Shutdown()
	{
		for (VkImageView swapchainImgView : swapchainImageViews)
		{
			vkDestroyImageView(device.GetDevice() , swapchainImgView, nullptr);
		}
		swapchainImageViews.clear();

		// destroy render-complete ssemaphores
		for (VkSemaphore& semaphore : renderCompleteSemaphores)
		{
			vkDestroySemaphore(device.GetDevice(), semaphore, nullptr);
		}

		renderCompleteSemaphores.clear();

		if (swapchain)
		{
			vkDestroySwapchainKHR(device.GetDevice(), swapchain, nullptr);
			swapchain = nullptr;
		}

		swapchainImages.clear();
	}
}
