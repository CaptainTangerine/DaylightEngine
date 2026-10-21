#pragma once
#include <vulkan/vulkan.h>

#include "ShaderCompiler.h"
#include "Graphics/VertexFormat.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanGraphicsPipeline;
	class VulkanComputePipeline;
	class VulkanBuffer;
	class VulkanImage;
	class VulkanSampler;

	class Renderer
	{
		constexpr static VkFormat depthStencilFormat = { VK_FORMAT_D32_SFLOAT_S8_UINT };
	public:
		Renderer(VulkanDevice& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		bool CreateSceneColorImage(uint32 width, uint32 height);
		void DestroySceneColorImage();

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

		void BeginSceneRendering(VkCommandBuffer commandBuffer);
		
		void CopyToPresentImage(VkCommandBuffer commandBuffer, VkImage finalImage);

	private:
		VulkanDevice& device;

		// SceneColor Image
		std::unique_ptr<VulkanImage> sceneColorImage = { nullptr };

		// Default depth-stencil images
		std::vector<std::unique_ptr<VulkanImage>> depthStencilImages;

		// Shader Compilier
		ShaderCompiler shaderCompiler;

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
		const std::vector<uint16> triangleIndices =
		{
			0, 1, 2
		};

		std::unique_ptr<VulkanBuffer> triangleVertexBuffer = { nullptr };
		std::unique_ptr<VulkanBuffer> triangleIndexBuffer = { nullptr };

		// 이미지를 매핑할 사각형 정점과 인덱스
		const std::vector<VertexFormat::FTexturedVertex> imageVertices =
		{
			{{-1.f, 1.f}, {0.f, 0.f}},
			{{-1.f, -1.f}, {0.f, 1.f}},
			{{1.f, -1.f}, {1.f, 1.f}},
			{{1.f, 1.f}, {1.f, 0.f}}
		};
		const std::vector<uint16> imageIndices =
		{
			0, 1, 2,
			0, 2, 3
		};

		std::unique_ptr<VulkanBuffer> imageVertexBuffer = { nullptr };
		std::unique_ptr<VulkanBuffer> imageIndexBuffer = { nullptr };

		// 이미지와 공용 샘플러
		std::unique_ptr<VulkanImage> testImage = { nullptr };
		std::unique_ptr<VulkanSampler> linearRepeatSampler = { nullptr };

		
		VkDescriptorSetLayout computeDescriptorSetLayout = { VK_NULL_HANDLE } ;
		VkDescriptorPool computeDescriptorPool = { VK_NULL_HANDLE };
		std::vector<VkDescriptorSet> computeDescriptorSets;

		std::vector<std::unique_ptr<VulkanImage>> sceneColorImages;
		std::vector<std::unique_ptr<VulkanImage>> computeOutputImages;

	};
}
