#include "pch.h"
#include "Renderer.h"
#include "GPUProfiler.h"
#include "Core/Paths.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Resources/TextureResource.h"
#include "Resources/ResourceManager.h"
#include "Rendering/RenderPass/BlurPass.h"
#include "Rendering/RenderPass/GeometryPass.h"
#include "Rendering/RenderPass/ImagePass.h"

namespace Dlight
{
	Renderer::Renderer(VulkanDevice& _device, ResourceManager& _resourceManager)
		: device(_device), resourceManager(_resourceManager), renderGraph(_device)
	{
		Initialize();
	}

	Renderer::~Renderer()
	{
		Shutdown();
	}

	void Renderer::Initialize()
	{
		blurPass = std::make_unique<Dlight::BlurPass>(device, resourceManager, shaderCompiler);

		const std::filesystem::path imagePath = Paths::GetAssetPath(L"Image/Test.jpg");
		FTextureRaw textureData = { Image(imagePath.string()), {} };
		testTexture = std::make_unique<TextureResource>("ImageTest", device, std::move(textureData));
		if (!testTexture->Load())
		{
			DL_LOG_ERROR("Failed to load test texture");
			std::abort();
		}

		geometryPass = std::make_unique<Dlight::GeometryPass>(device, resourceManager, shaderCompiler);
		imagePass = std::make_unique<Dlight::ImagePass>(device, resourceManager, shaderCompiler);
		imagePass->SetTexture(*testTexture);

		BuildRenderGraph();
	}

	void Renderer::BuildRenderGraph()
	{
		renderGraph.Reset();

		const VkExtent2D renderExtent = {
			device.GetSwapchain().GetWidth(),
			device.GetSwapchain().GetHeight()
		};

		const RenderGraphImageHandle sceneColor = AddImagePass(renderExtent);
		const FBlurGraphOutput blurOutput = AddBlurPass(sceneColor, renderExtent);
		AddPresentPass(blurOutput.output, renderExtent);

		renderGraph.Compile();

		blurPass->SetResources(
			renderGraph.GetImageView(sceneColor),
			renderGraph.GetImageView(blurOutput.temp),
			renderGraph.GetImageView(blurOutput.output));
	}

	RenderGraphImageHandle Renderer::AddImagePass(VkExtent2D renderExtent)
	{
		FVulkanImageDesc sceneColorDesc = {};
		sceneColorDesc.format = VulkanSwapchain::swapchainFormat;
		sceneColorDesc.extent = { renderExtent.width, renderExtent.height, 1 };
		sceneColorDesc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		struct FImagePassData
		{
			RenderGraphImageHandle sceneColor;
		};

		const FImagePassData imagePassData = renderGraph.AddPass<FImagePassData>(
			"ImagePass",
			[sceneColorDesc](RenderGraph::Builder& builder, FImagePassData& data)
			{
				data.sceneColor = builder.DeclareInternal(sceneColorDesc);
				data.sceneColor = builder.Write(data.sceneColor, EImageState::ColorAttachmentWrite);
			},
			[this, renderExtent](VkCommandBuffer commandBuffer, const FImagePassData& data)
			{
				imagePass->Execute(
					commandBuffer,
					renderGraph.GetImageView(data.sceneColor),
					renderExtent);
			});

		return imagePassData.sceneColor;
	}

	Renderer::FBlurGraphOutput Renderer::AddBlurPass(RenderGraphImageHandle input, VkExtent2D renderExtent)
	{
		FVulkanImageDesc blurImageDesc = {};
		blurImageDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		blurImageDesc.extent = { renderExtent.width, renderExtent.height, 1 };
		blurImageDesc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		struct FBlurHorizontalPassData
		{
			RenderGraphImageHandle sceneColor;
			RenderGraphImageHandle blurTemp;
		};

		const FBlurHorizontalPassData horizontalPassData = renderGraph.AddPass<FBlurHorizontalPassData>(
			"BlurHorizontalPass",
			[input, blurImageDesc](RenderGraph::Builder& builder, FBlurHorizontalPassData& data)
			{
				data.sceneColor = builder.Read(input, EImageState::ComputeSampledRead);
				data.blurTemp = builder.DeclareInternal(blurImageDesc);
				data.blurTemp = builder.Write(data.blurTemp, EImageState::ComputeStorageWrite);
			},
			[this, renderExtent](VkCommandBuffer commandBuffer, const FBlurHorizontalPassData&)
			{
				GPUProfiler::Scope scope(device.GetGPUProfiler(), "Blur");
				blurPass->ExecuteHorizontal(commandBuffer, renderExtent);
			});

		struct FBlurVerticalPassData
		{
			RenderGraphImageHandle blurTemp;
			RenderGraphImageHandle blurOutput;
		};

		const FBlurVerticalPassData verticalPassData = renderGraph.AddPass<FBlurVerticalPassData>(
			"BlurVerticalPass",
			[blurTemp = horizontalPassData.blurTemp, blurImageDesc](RenderGraph::Builder& builder, FBlurVerticalPassData& data)
			{
				data.blurTemp = builder.Read(blurTemp, EImageState::ComputeSampledRead);
				data.blurOutput = builder.DeclareInternal(blurImageDesc);
				data.blurOutput = builder.Write(data.blurOutput, EImageState::ComputeStorageWrite);
			},
			[this, renderExtent](VkCommandBuffer commandBuffer, const FBlurVerticalPassData&)
			{
				GPUProfiler::Scope scope(device.GetGPUProfiler(), "Blur");
				blurPass->ExecuteVertical(commandBuffer, renderExtent);
			});

		return {
			horizontalPassData.blurTemp,
			verticalPassData.blurOutput
		};
	}

	void Renderer::AddPresentPass(RenderGraphImageHandle input, VkExtent2D renderExtent)
	{
		FVulkanImageDesc backBufferDesc = {};
		backBufferDesc.format = VulkanSwapchain::swapchainFormat;
		backBufferDesc.extent = { renderExtent.width, renderExtent.height, 1 };
		backBufferDesc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		struct FPresentPassData
		{
			RenderGraphImageHandle blurOutput;
			RenderGraphImageHandle backBuffer;
		};

		renderGraph.AddPass<FPresentPassData>(
			"PresentPass",
			[this, input, backBufferDesc](RenderGraph::Builder& builder, FPresentPassData& data)
			{
				data.blurOutput = builder.Read(input, EImageState::CopySource);
				data.backBuffer = builder.DeclareExternal(backBufferDesc, EImageState::Undefined, EImageState::Present);
				renderGraphBackBuffer = data.backBuffer;
				data.backBuffer = builder.Write(data.backBuffer, EImageState::CopyDestination);
			},
			[this, renderExtent](VkCommandBuffer commandBuffer, const FPresentPassData& data)
			{
				const int32 width = static_cast<int32>(renderExtent.width);
				const int32 height = static_cast<int32>(renderExtent.height);

				VkImageBlit blitRegion = {};
				blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitRegion.srcSubresource.layerCount = 1;
				blitRegion.srcOffsets[1] = { width, height, 1 };
				blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blitRegion.dstSubresource.layerCount = 1;
				blitRegion.dstOffsets[1] = { width, height, 1 };

				vkCmdBlitImage(
					commandBuffer,
					renderGraph.GetImage(data.blurOutput),
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					renderGraph.GetImage(data.backBuffer),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&blitRegion,
					VK_FILTER_NEAREST);
			});
	}

	void Renderer::Shutdown()
	{
		geometryPass.reset();
		imagePass.reset();
		blurPass.reset();

		testTexture.reset();
	}

	void Renderer::OnResize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		BuildRenderGraph();
	}

	void Renderer::Render()
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();
		const VkCommandBuffer commandBuffer = frameResource.commandBuffer;

		GPUProfiler& profiler = device.GetGPUProfiler();
		GPUProfiler::Scope frameScope(profiler, "Render");

		renderGraph.BindExternal(renderGraphBackBuffer, device.GetCurrentSwapchainImage());
		renderGraph.Execute(commandBuffer);
	}
}
