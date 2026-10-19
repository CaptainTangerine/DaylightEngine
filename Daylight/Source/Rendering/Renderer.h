#pragma once
#include "ShaderCompiler.h"
#include "Graphics/VertexFormat.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanPipeline;
	class MeshResource;
	class TextureResource;
	class ShaderResource;

	class Renderer
	{
	public:
		Renderer(VulkanDevice& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		bool CreateTrianglePipelines();
		bool CreateImageTestPipelines();

		void DestroyPipelines();

		void Render();
	private:
		void Initialize();
		void Shutdown();

	private:
		VulkanDevice& device;

		// Shader Compilier
		ShaderCompiler shaderCompiler;

		// Shader Resource
		std::unique_ptr<ShaderResource> triangleVertexShader = { nullptr };
		std::unique_ptr<ShaderResource> triangleFragmentShader = { nullptr };
		std::unique_ptr<ShaderResource> imageVertexShader = { nullptr };
		std::unique_ptr<ShaderResource> imageFragmentShader = { nullptr };

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
		const std::vector<uint32> triangleIndices =
		{
			0, 1, 2
		};

		std::unique_ptr<MeshResource> triangleMesh = { nullptr };

		// 이미지를 매핑할 사각형 정점과 인덱스
		const std::vector<VertexFormat::FTexturedVertex> imageVertices =
		{
			{{-0.5f, 0.5f}, {0.f, 0.f}},
			{{-0.5f, -0.5f}, {0.f, 1.f}},
			{{0.5f, -0.5f}, {1.f, 1.f}},
			{{0.5f, 0.5f}, {1.f, 0.f}}
		};
		const std::vector<uint32> imageIndices =
		{
			0, 1, 2,
			0, 2, 3
		};

		std::unique_ptr<MeshResource> imageMesh = { nullptr };

		// 테스트 텍스처
		std::unique_ptr<TextureResource> testTexture = { nullptr };

	};
}
