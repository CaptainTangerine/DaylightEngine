#pragma once

#include "Graphics/VertexFormat.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class MeshResource;
	class ResourceManager;
	class ShaderCompiler;
	class ShaderResource;
	class TextureResource;
	class VulkanDevice;
	class VulkanGraphicsPipeline;

	class ImagePass
	{
	public:
		ImagePass(VulkanDevice& device, ResourceManager& resourceManager, ShaderCompiler& shaderCompiler);
		~ImagePass();

		ImagePass(const ImagePass&) = delete;
		ImagePass& operator=(const ImagePass&) = delete;

		void SetTexture(TextureResource& texture);
		void Execute(VkCommandBuffer commandBuffer, VkImageView renderTargetView, VkExtent2D extent);

	private:
		void CreateMesh();
		void CreatePipeline();

		VulkanDevice& device;
		ResourceManager& resourceManager;
		ShaderCompiler& shaderCompiler;

		const std::vector<VertexFormat::FTexturedVertex> vertices =
		{
			{{-1.f, 1.f}, {0.f, 0.f}},
			{{-1.f, -1.f}, {0.f, 1.f}},
			{{1.f, -1.f}, {1.f, 1.f}},
			{{1.f, 1.f}, {1.f, 0.f}}
		};

		const std::vector<uint32> indices =
		{
			0, 1, 2,
			0, 2, 3
		};

		std::unique_ptr<MeshResource> mesh;
		std::unique_ptr<ShaderResource> vertexShader;
		std::unique_ptr<ShaderResource> fragmentShader;
		std::unique_ptr<VulkanGraphicsPipeline> pipeline;
	};
}
