#pragma once
#include "ShaderCompiler.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanPipeline;

	class Renderer
	{
	public:
		Renderer(VulkanDevice& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		bool CreateDepthStencilResources();
		void DestroyDepthStencilResources();

		bool CreatePipelines();
		void DestroyPipelines();

	private:
		void Initialize();
		void Shutdown();

	private:
		VulkanDevice& device;
		
		// Depth-stencil buffer
		static constexpr VkFormat depthStencilFormat{ VK_FORMAT_D32_SFLOAT_S8_UINT };
		VkImage depthStencilImage = { nullptr };
		VkImageView depthStencilImageView = { nullptr };
		VmaAllocation depthStencilImageAllocation = { nullptr };

		// Shader Compilier
		ShaderCompiler shaderCompiler;

		// Pipeline
		std::unique_ptr<VulkanPipeline> trianglePipeline = { nullptr };

	};
}
