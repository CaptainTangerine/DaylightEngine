#pragma once
#include <vulkan/vulkan.h>

#include "ShaderCompiler.h"
#include "RenderGraph/RenderGraph.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class TextureResource;
	class ResourceManager;
	class BlurPass;
	class GeometryPass;
	class ImagePass;

	class Renderer
	{
	public:
		Renderer(VulkanDevice& device, ResourceManager& resourceManager);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

	public:
		void OnResize(uint32 width, uint32 height);
		void Render();

	private:
		struct FBlurGraphOutput
		{
			RenderGraphImageHandle temp;
			RenderGraphImageHandle output;
		};

		void Initialize();
		void BuildRenderGraph();

		RenderGraphImageHandle AddImagePass(VkExtent2D renderExtent);
		FBlurGraphOutput AddBlurPass(RenderGraphImageHandle input, VkExtent2D renderExtent);
		void AddPresentPass(RenderGraphImageHandle input, VkExtent2D renderExtent);

		void Shutdown();

	private:
		VulkanDevice& device;
		ResourceManager& resourceManager;
		RenderGraph renderGraph;

		ShaderCompiler shaderCompiler;
		RenderGraphImageHandle renderGraphBackBuffer = {};

		std::unique_ptr<Dlight::GeometryPass> geometryPass = { nullptr };
		std::unique_ptr<Dlight::ImagePass> imagePass = { nullptr };
		std::unique_ptr<Dlight::BlurPass> blurPass = { nullptr };

		// 테스트 텍스처
		std::unique_ptr<TextureResource> testTexture = { nullptr };

	};
}
