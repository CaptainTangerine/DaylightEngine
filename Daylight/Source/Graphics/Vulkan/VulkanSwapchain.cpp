#include "pch.h"
#include "VulkanSwapchain.h"

namespace Dlight
{
	VulkanSwapchain::VulkanSwapchain(VulkanDevice& device)
		: device(device)
	{
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		Shutdown();
	}

	bool VulkanSwapchain::Initialize(uint32_t width, uint32_t height)
	{
		swapchainWidth  = width;
		swapchainHeight = height;

		return true;
	}

	void VulkanSwapchain::Shutdown()
	{

	}
}
