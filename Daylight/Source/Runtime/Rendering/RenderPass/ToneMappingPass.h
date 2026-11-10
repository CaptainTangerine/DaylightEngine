#pragma once

#include <memory>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class ShaderCompiler;
	class ShaderResource;
	class VulkanDevice;
	class VulkanGraphicsPipeline;
	class VulkanSampler;

	class ToneMappingPass
	{
	public:
		ToneMappingPass(VulkanDevice& device, ShaderCompiler& shaderCompiler);
		~ToneMappingPass();

		ToneMappingPass(const ToneMappingPass&) = delete;
		ToneMappingPass& operator=(const ToneMappingPass&) = delete;

		void SetSource(VkImageView sourceView);
		void Execute(VkCommandBuffer commandBuffer, VkImageView renderTargetView, VkExtent2D extent);

	private:
		void CreateDescriptors();
		void CreatePipeline();
		void DestroyDescriptors();

		VulkanDevice& device;
		ShaderCompiler& shaderCompiler;

		std::unique_ptr<ShaderResource> vertexShader;
		std::unique_ptr<ShaderResource> fragmentShader;
		std::unique_ptr<VulkanGraphicsPipeline> pipeline;
		std::unique_ptr<VulkanSampler> sampler;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	};
}
