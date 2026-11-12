#pragma once

#include "Rendering/ViewData.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class ShaderCompiler;
	class ShaderResource;
	class VulkanBuffer;
	class VulkanDevice;
	class VulkanGraphicsPipeline;
	class VulkanImage;
	class VulkanSampler;

	class SkyPass
	{
	public:
		static constexpr VkFormat ColorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

		SkyPass(VulkanDevice& device, ShaderCompiler& shaderCompiler);
		~SkyPass();

		SkyPass(const SkyPass&) = delete;
		SkyPass& operator=(const SkyPass&) = delete;

		void SetCubemap(const VulkanImage& cubemap);
		void Execute(
			VkCommandBuffer commandBuffer,
			VkImageView sceneColorView,
			VkExtent2D extent,
			const FViewData& viewData);

	private:
		void CreateDescriptors();
		void DestroyDescriptors();
		void CreatePipeline();
		void UpdateViewData(const FViewData& viewData);

	private:
		VulkanDevice& device;
		ShaderCompiler& shaderCompiler;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<std::unique_ptr<VulkanBuffer>> viewUniformBuffers;

		std::unique_ptr<ShaderResource> vertexShader;
		std::unique_ptr<ShaderResource> fragmentShader;
		std::unique_ptr<VulkanGraphicsPipeline> pipeline;
		std::unique_ptr<VulkanSampler> sampler;
	};
}
