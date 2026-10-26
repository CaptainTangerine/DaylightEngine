#pragma once
#include <vulkan/vulkan.h>

#include "ShaderCompiler.h"
#include "Graphics/VertexFormat.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class MeshResource;
	class TextureResource;
	class ShaderResource;
	class VulkanGraphicsPipeline;
	class VulkanComputePipeline;
	class VulkanImage;

	class Renderer
	{
		constexpr static VkFormat depthStencilFormat = { VK_FORMAT_D32_SFLOAT_S8_UINT };
	public:
		Renderer(VulkanDevice& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		bool CreateSceneColorImages(uint32 width, uint32 height);
		void DestroySceneColorImages();

		bool CreateDepthStencilImages(uint32 width, uint32 height);
		void DestroyDepthStencilImages();

		void OnResize(uint32 width, uint32 height);

		bool CreateTrianglePipelines();
		bool CreateImageTestPipelines();
		bool CreateBlurPipeline();

		bool CreateComputeDescriptorSets();
		void UpdateComputeDescriptorSets();
		void DestroyComputeDescriptors();

		void DestroyPipelines();

		void Render();

	private:
		void Initialize();
		void Shutdown();

		void TransitionScenePassAttachments(VkCommandBuffer commandBuffer);
		void ScenePass(VkCommandBuffer commandBuffer);

		void TransitionHorizontalBlurPassImages(VkCommandBuffer commandBuffer);
		void TransitionVerticalBlurPassImages(VkCommandBuffer commandBuffer);
		void BlurPass(VkCommandBuffer commandBuffer);
		
		void TransitionPresentPassImages(VkCommandBuffer commandBuffer);
		void CopyToPresentImage(VkCommandBuffer commandBuffer, VkImage finalImage);

	private:
		VulkanDevice& device;

		// SceneColor Image
		std::vector<std::unique_ptr<VulkanImage>> sceneColorImages;

		std::vector<std::unique_ptr<VulkanImage>> horizontalBlurImages;

		// ComputeOuput Image
		std::vector<std::unique_ptr<VulkanImage>> computeOutputImages;

		// Default depth-stencil images
		std::vector<std::unique_ptr<VulkanImage>> depthStencilImages;

		// Shader Compilier
		ShaderCompiler shaderCompiler;

		// Shader Resource
		std::unique_ptr<ShaderResource> triangleVertexShader = { nullptr };
		std::unique_ptr<ShaderResource> triangleFragmentShader = { nullptr };
		std::unique_ptr<ShaderResource> imageVertexShader = { nullptr };
		std::unique_ptr<ShaderResource> imageFragmentShader = { nullptr };

		// Pipeline
		std::unique_ptr<VulkanGraphicsPipeline> trianglePipeline = { nullptr };
		std::unique_ptr<VulkanGraphicsPipeline> imageTestPipeline = { nullptr };

		std::unique_ptr<VulkanComputePipeline> blurPipeline = { nullptr };


		// 색상 삼각형 정점과 인덱스
		const std::vector<VertexFormat::FColoredVertex> triangleVertices =
		{
			{{0.f, 0.5f}, {1.0f, 0.0f, 0.0f}},
			{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		const std::vector<uint32> triangleIndices =
		{
			0, 1, 2
		};

		std::unique_ptr<MeshResource> triangleMesh = { nullptr };

		// 이미지를 매핑할 사각형 정점과 인덱스
		const std::vector<VertexFormat::FTexturedVertex> imageVertices =
		{
			{{-1.f, 1.f}, {0.f, 0.f}},
			{{-1.f, -1.f}, {0.f, 1.f}},
			{{1.f, -1.f}, {1.f, 1.f}},
			{{1.f, 1.f}, {1.f, 0.f}}
		};
		const std::vector<uint32> imageIndices =
		{
			0, 1, 2,
			0, 2, 3
		};

		std::unique_ptr<MeshResource> imageMesh = { nullptr };

		// 테스트 텍스처
		std::unique_ptr<TextureResource> testTexture = { nullptr };

		
		VkDescriptorSetLayout computeDescriptorSetLayout = { VK_NULL_HANDLE } ;
		VkDescriptorPool computeDescriptorPool = { VK_NULL_HANDLE };
		std::vector<VkDescriptorSet> horizontalBlurDescriptorSets;
		std::vector<VkDescriptorSet> verticalBlurDescriptorSets;
	};
}
