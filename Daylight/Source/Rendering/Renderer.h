#pragma once
#include "ShaderCompiler.h"
#include "Graphics/VertexFormat.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanPipeline;
	class VulkanVertexBuffer;

	class Renderer
	{
	public:
		Renderer(VulkanDevice& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		bool CreateTrianglePipelines();
		void DestroyPipelines();

		void Render();
	private:
		void Initialize();
		void Shutdown();

	private:
		VulkanDevice& device;

		// Shader Compilier
		ShaderCompiler shaderCompiler;

		// Pipeline
		std::unique_ptr<VulkanPipeline> trianglePipeline = { nullptr };
		
		//Triangle VertexBuffer
		const std::vector<VertexFormat::ColoredVertex> triangleVertices =
		{
			{{0.f, 0.5f}, {1.0f, 0.0f, 0.0f}},
			{{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		std::unique_ptr<VulkanVertexBuffer> triangleVertexBuffer = { nullptr }; 
	};
}
