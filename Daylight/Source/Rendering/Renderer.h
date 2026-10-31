#pragma once
#include <vulkan/vulkan.h>

#include "ShaderCompiler.h"
#include "RenderGraph/RenderGraph.h"

namespace Dlight
{
	class VulkanDevice;
	class VulkanSwapchain;
	class TextureResource;
	class VulkanImage;
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
		void Initialize();
		void BuildRenderGraph();

		void Shutdown();

		void TransitionPresentPassImages(VkCommandBuffer commandBuffer);
		void CopyToPresentImage(VkCommandBuffer commandBuffer, VkImage finalImage);

	private:
		VulkanDevice& device;
		ResourceManager& resourceManager;
		RenderGraph renderGraph;

		ShaderCompiler shaderCompiler;
		RenderGraphImageHandle renderGraphBackBuffer = {};

		std::unique_ptr<Dlight::GeometryPass> geometryPass = { nullptr };
		std::unique_ptr<Dlight::ImagePass> imagePass = { nullptr };
		std::unique_ptr<Dlight::BlurPass> blurPass = { nullptr };

		// SceneColor Image
		std::vector<std::unique_ptr<VulkanImage>> sceneColorImages;

		std::vector<std::unique_ptr<VulkanImage>> horizontalBlurImages;

		// ComputeOuput Image
		std::vector<std::unique_ptr<VulkanImage>> computeOutputImages;

		// Default depth-stencil images
		std::vector<std::unique_ptr<VulkanImage>> depthStencilImages;

		// 테스트 텍스처
		std::unique_ptr<TextureResource> testTexture = { nullptr };

	};
}
