#pragma once
#include "vulkan/vulkan.h"
#include "Rendering/ShaderConventions.h"
#include <vector>

namespace Dlight
{
	struct FGraphicsPipelineDesc
	{
		VkShaderModule vertexShader = VK_NULL_HANDLE;
		VkShaderModule fragmentShader = VK_NULL_HANDLE;

		const char* vertexEntryPoint = ShaderConventions::VertexEntry;
		const char* fragmentEntryPoint = ShaderConventions::PixelEntry;

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

		std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;

		VkFormat colorFormat = VK_FORMAT_UNDEFINED;
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	};

	struct FComputePipelineDesc
	{
		VkShaderModule computeShader = VK_NULL_HANDLE;
		const char* computeEntryPoint = ShaderConventions::ComputeEntry;

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	};

	class VulkanDevice;

	class VulkanGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline(VulkanDevice& _device, const FGraphicsPipelineDesc& desc);
		~VulkanGraphicsPipeline();

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline& other) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline& other) = delete;

		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; };
		VkPipeline GetPipeline() const { return pipeline; }
		
	private:
		void Initialize(const FGraphicsPipelineDesc& desc);
		void Shutdown();

	private:
		VulkanDevice& device;

		VkPipelineLayout pipelineLayout = { nullptr };
		VkPipeline pipeline = { nullptr };
	};

	class VulkanComputePipeline
	{
	public:
		VulkanComputePipeline(VulkanDevice& _device, const FComputePipelineDesc& desc);
		~VulkanComputePipeline();

		VulkanComputePipeline(const VulkanComputePipeline& other) = delete;
		VulkanComputePipeline& operator=(const VulkanComputePipeline& other) = delete;

		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
		VkPipeline GetPipeline() const { return pipeline; }

	private:
		void Initialize(const FComputePipelineDesc& desc);
		void Shutdown();

	private:
		VulkanDevice& device;

		VkPipelineLayout pipelineLayout = { nullptr };
		VkPipeline pipeline = { nullptr };
	};
}
