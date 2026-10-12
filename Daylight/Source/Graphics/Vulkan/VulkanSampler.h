#pragma once

#include "vulkan/vulkan.h"

namespace Dlight
{
	class VulkanDevice;

	class VulkanSampler
	{
	public:
		VulkanSampler(VulkanDevice& _device, const VkSamplerCreateInfo& createInfo);
		~VulkanSampler();

		VulkanSampler(const VulkanSampler&) = delete;
		VulkanSampler& operator=(const VulkanSampler&) = delete;
		VulkanSampler(VulkanSampler&&) = delete;
		VulkanSampler& operator=(VulkanSampler&&) = delete;

	public:
		VkSampler GetSampler() const { return sampler; };

	private:
		VulkanDevice& device;

		VkSampler sampler = { VK_NULL_HANDLE };
	};
}
