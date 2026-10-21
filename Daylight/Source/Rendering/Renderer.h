#pragma once
#include <vulkan/vulkan.h>

#include "ShaderCompiler.h"
#include "Graphics/VertexFormat.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanPipeline;
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

		bool CreateDepthStencilImage(uint32 width, uint32 height);
		void DestroyDepthStencilImage();
		void OnResize(uint32 width, uint32 height);

		bool CreateTrianglePipelines();
		bool CreateImageTestPipelines();

		void DestroyPipelines();

		void Render();
	private:
		void Initialize();
		void Shutdown();

	private:
		VulkanDevice& device;

		// SceneColor buffer
		std::unique_ptr<VulkanImage> sceneColorImage = { nullptr };

		// Default depthstencil buffer
		std::unique_ptr<VulkanImage> depthStencilImage = { nullptr };

		// Shader Compilier
		ShaderCompiler shaderCompiler;

		// Pipeline
		std::unique_ptr<VulkanPipeline> trianglePipeline = { nullptr };
		std::unique_ptr<VulkanPipeline> imageTestPipeline = { nullptr };

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

	};
}
