#pragma once
#include "vulkan/vulkan.h"
#include "Rendering/ShaderConventions.h"
#include <vector>

namespace Dlight
{
	struct GfxPipelineDesc
	{
		VkShaderModule vertexShader = VK_NULL_HANDLE;
		VkShaderModule fragmentShader = VK_NULL_HANDLE;

		const char* vertexEntryPoint = ShaderConventions::VertexEntry;
		const char* fragmentEntryPoint = ShaderConventions::PixelEntry;

		std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;

		VkFormat colorFormat = VK_FORMAT_UNDEFINED;
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	};

	class VulkanDevice;

	class VulkanPipeline
	{
	public:
		VulkanPipeline(VulkanDevice& _device, const GfxPipelineDesc& desc);
		~VulkanPipeline();

		VulkanPipeline(const VulkanPipeline& other) = delete;
		VulkanPipeline& operator= (const VulkanPipeline & other) = delete;

		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; };
		VkPipeline GetPipeline() const { return pipeline; }
		
	private:
		void Initialize(const GfxPipelineDesc& desc);
		void Shutdown();

	private:
		VulkanDevice& device;

		VkPipelineLayout pipelineLayout = { nullptr };
		VkPipeline pipeline = { nullptr };
	};
}
