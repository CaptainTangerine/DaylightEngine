#pragma once

#include "Graphics/VertexFormat.h"
#include "Rendering/ViewData.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace Dlight
{
	class MeshResource;
	class ResourceManager;
	class ShaderCompiler;
	class ShaderResource;
	class VulkanBuffer;
	class VulkanDevice;
	class VulkanGraphicsPipeline;

	class GeometryPass
	{
	public:
		static constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

		struct alignas(16) FPerViewUniform
		{
			Matrix viewMatrix;
			Matrix projectionMatrix;
		};

		GeometryPass(VulkanDevice& device, ResourceManager& resourceManager, ShaderCompiler& shaderCompiler);
		~GeometryPass();

		void Execute(
			VkCommandBuffer commandBuffer,
			VkImageView sceneColorView,
			VkImageView sceneDepthView,
			VkExtent2D extent,
			const FViewData& viewData);

	private:
		void CreateViewResources();
		void DestroyViewResources();
		void CreateMesh();
		void CreatePipeline();
		void UpdateViewData(const FViewData& viewData);

		VulkanDevice& device;
		ResourceManager& resourceManager;
		ShaderCompiler& shaderCompiler;


		const std::vector<VertexFormat::FColoredVertex> vertices =
		{
			{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
			{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
			{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
			{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
			{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
			{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.5f, 0.0f}}
		};

		const std::vector<uint32> indices =
		{
			0, 3, 2, 0, 2, 1,
			4, 5, 6, 4, 6, 7,
			0, 4, 7, 0, 7, 3,
			1, 2, 6, 1, 6, 5,
			0, 1, 5, 0, 5, 4,
			3, 7, 6, 3, 6, 2
		};

		std::unique_ptr<MeshResource> mesh;

		VkDescriptorSetLayout viewDescriptorSetLayout = { VK_NULL_HANDLE };
		VkDescriptorPool viewDescriptorPool = { VK_NULL_HANDLE } ;
		std::vector<VkDescriptorSet> viewDescriptorSets;
		std::vector<std::unique_ptr<VulkanBuffer>> viewUniformBuffers;

		std::unique_ptr<ShaderResource> vertexShader;
		std::unique_ptr<ShaderResource> fragmentShader;
		std::unique_ptr<VulkanGraphicsPipeline> pipeline;
	};
}
