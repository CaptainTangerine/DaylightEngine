#include "pch.h"
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include <stdexcept>

namespace Dlight
{
	VulkanSwapchain::VulkanSwapchain(VulkanDevice& _device, uint32 width, uint32 height)
		: device(_device)
	{
		try
		{
			Initialize(width, height);
		}
		catch (...)
		{
			Shutdown();
			throw;
		}
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		Shutdown();
	}

	VkPresentModeKHR VulkanSwapchain::ChoosePresentMode() const
	{
		uint32 presentModeCount = 0;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			device.GetPhysicalDevice(),
			device.GetSurface(),
			&presentModeCount,
			nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to get present mode count");
		}

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(
			device.GetPhysicalDevice(),
			device.GetSurface(),
			&presentModeCount,
			presentModes.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to get present modes");
		}

		for (VkPresentModeKHR presentMode : presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				return presentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	bool VulkanSwapchain::Initialize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0)
		{
			return false;
		}

		// 지원되는 포맷인지 검사
		uint32 formatCount = 0;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), device.GetSurface(), &formatCount, nullptr) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to get surface format count");
		}
		if (formatCount == 0)
		{
			throw std::runtime_error("No surface formats are available");
		}
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetPhysicalDevice(), device.GetSurface(), &formatCount, surfaceFormats.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to get surface formats");
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
			throw std::runtime_error("Requested swapchain format is not supported by the surface");
		}


		VkSurfaceCapabilitiesKHR surfaceCaps{};
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetPhysicalDevice(), device.GetSurface(), &surfaceCaps))
		{
			throw std::runtime_error("Can't get the surface capabilities");
		}

		if (!(surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			throw std::runtime_error("Swapchain does not support transfer destination usage");
		}

		VkExtent2D imageExtent{};
		if (surfaceCaps.currentExtent.width != UINT32_MAX)
		{
			// 고정 크기 surface에서는 이벤트의 크기 대신 Vulkan이 조회한 현재 크기를 사용한다.
			imageExtent = surfaceCaps.currentExtent;
		}
		else
		{
			// UINT32_MAX는 애플리케이션이 허용 범위 안에서 크기를 선택할 수 있다는 뜻이다.
			imageExtent.width = std::clamp(width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
			imageExtent.height = std::clamp(height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);
		}
		// 최소화 시 이벤트의 크기가 남아 있어도 Vulkan surface는 0x0일 수 있다.
		if (imageExtent.width == 0 || imageExtent.height == 0)
		{
			return false;
		}

		// 최종 크기를 확인한 뒤에만 기존 스왑체인을 해제한다.
		Shutdown();
		swapchainWidth = imageExtent.width;
		swapchainHeight = imageExtent.height;

		// 스왑체인 생성
		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		uint32 imageCount = std::max(3u, surfaceCaps.minImageCount);
		if (surfaceCaps.maxImageCount > 0)
		{
			imageCount = std::min(imageCount, surfaceCaps.maxImageCount);
		}

		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = device.GetSurface();
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = swapchainFormat;
		swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		swapchainCreateInfo.imageExtent = imageExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = ChoosePresentMode();

		if (swapchainCreateInfo.presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			DL_LOG_INFO("Swapchain present mode: IMMEDIATE (VSync off)");
		}
		else
		{
			DL_LOG_WARN("IMMEDIATE present mode is unavailable; falling back to FIFO");
		}
		
		if (vkCreateSwapchainKHR(device.GetDevice(), &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("Error creating swapchain");
		}

		// 실제 스왑체인이 관리하는 이미지 개수 가져오기
		imageCount = 0;
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
				throw std::runtime_error("Error Createing Swapchian ImageViews");
			}
		}

		renderCompleteSemaphores.resize(imageCount);

		for (VkSemaphore& semaphore : renderCompleteSemaphores)
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (VK_SUCCESS != vkCreateSemaphore(device.GetDevice(), &semaphoreInfo, nullptr, &semaphore))
			{
				throw std::runtime_error("Error create render complete semaphore");
			}
		}
		return true;
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
