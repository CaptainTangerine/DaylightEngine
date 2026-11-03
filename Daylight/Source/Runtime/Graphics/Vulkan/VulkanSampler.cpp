#include "pch.h"
#include "VulkanSampler.h"
#include "VulkanDevice.h"

namespace Dlight
{
	VulkanSampler::VulkanSampler(VulkanDevice& _device, const VkSamplerCreateInfo& createInfo)
		: device(_device)
	{
		if (VK_SUCCESS != vkCreateSampler(device.GetDevice(), &createInfo, nullptr, &sampler))
		{
			DL_LOG_ERROR("Failed to create texture sampler");
			std::abort();
		}
	}

	VulkanSampler::~VulkanSampler()
	{
		if (VK_NULL_HANDLE != sampler)
		{
			vkDestroySampler(device.GetDevice(), sampler, nullptr);
		}
	}
}
