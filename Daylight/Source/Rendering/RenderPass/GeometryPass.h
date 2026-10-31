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
	class VulkanDevice;
	class VulkanGraphicsPipeline;

	class GeometryPass
	{
	public:
		GeometryPass(VulkanDevice& device, ResourceManager& resourceManager, ShaderCompiler& shaderCompiler);
		~GeometryPass();

		void Execute(VkCommandBuffer commandBuffer, VkImageView renderTargetView, VkExtent2D extent);

	private:
		void CreateMesh();
		void CreatePipeline();

		VulkanDevice& device;
		ResourceManager& resourceManager;
		ShaderCompiler& shaderCompiler;

		const std::vector<VertexFormat::FColoredVertex> vertices =
		{
			{{0.f, 0.5f}, {1.0f, 0.0f, 0.0f}},
			{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		const std::vector<uint32> indices = { 0, 1, 2 };

		std::unique_ptr<MeshResource> mesh;
		std::unique_ptr<ShaderResource> vertexShader;
		std::unique_ptr<ShaderResource> fragmentShader;
		std::unique_ptr<VulkanGraphicsPipeline> pipeline;
	};
}
