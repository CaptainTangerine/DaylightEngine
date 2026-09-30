#pragma once
#include "ShaderCompiler.h"

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
		bool CreatePipelines();
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
	};
}
