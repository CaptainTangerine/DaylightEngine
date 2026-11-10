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
			Vector3 cameraPosition;
		};

		struct alignas(16) FLightUniform
		{
			Vector4 positions[4];
			Vector4 colors[4];
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
		void CreateUniformResources();
		void DestroyUniformResources();
		
		void CreateMesh();
		void CreateImages();

		void CreatePipelineUnLit();
		void CreatePipelinePBR();
		void UpdateViewData(const FViewData& viewData);

		VulkanDevice& device;
		ResourceManager& resourceManager;
		ShaderCompiler& shaderCompiler;


		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		std::vector<VkDescriptorSet> descriptorSets;

		std::vector<std::unique_ptr<VulkanBuffer>> viewUniformBuffers;
		std::vector<std::unique_ptr<VulkanBuffer>> lightUniformBuffers;

		std::unique_ptr<ShaderResource> vertexShaderUnLit;
		std::unique_ptr<ShaderResource> fragmentShaderUnLit;
		std::unique_ptr<ShaderResource> vertexShaderPBR;
		std::unique_ptr<ShaderResource> fragmentShaderPBR;
		std::unique_ptr<VulkanGraphicsPipeline> pipelineUnLit;
		std::unique_ptr<VulkanGraphicsPipeline> pipelinePBR;


		std::unique_ptr<MeshResource> mesh;

		// Obsidian Test
		std::unique_ptr<VulkanImage> abedoMap;
		std::unique_ptr<VulkanImage> normalMap;
		std::unique_ptr<VulkanImage> heightMap;
		std::unique_ptr<VulkanImage> metalicMap;
		std::unique_ptr<VulkanImage> roughnessMap;
		std::unique_ptr<VulkanImage> AmbientOcclusionMap;


	};
}
