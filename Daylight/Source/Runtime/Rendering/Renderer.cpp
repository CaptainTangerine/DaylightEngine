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
#include "Rendering/RenderPass/ToneMappingPass.h"

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
		//blurPass = std::make_unique<Dlight::BlurPass>(device, resourceManager, shaderCompiler);

		const std::filesystem::path imagePath = Paths::GetAssetPath(L"Image/Test.jpg");
		testTexture = std::make_unique<TextureResource>("ImageTest", device, imagePath);
		if (!testTexture->Load())
		{
			DL_LOG_ERROR("Failed to load test texture");
			std::abort();
		}

		geometryPass = std::make_unique<Dlight::GeometryPass>(device, resourceManager, shaderCompiler);
		imagePass = std::make_unique<Dlight::ImagePass>(device, resourceManager, shaderCompiler);
		imagePass->SetTexture(*testTexture);
		toneMappingPass = std::make_unique<Dlight::ToneMappingPass>(device, shaderCompiler);

		BuildRenderGraph();
	}

	void Renderer::BuildRenderGraph()
	{
		renderGraph.Reset();

		const VkExtent2D renderExtent = {
			device.GetSwapchain().GetWidth(),
			device.GetSwapchain().GetHeight()
		};

		//const RenderGraphImageHandle sceneColor = AddImagePass(renderExtent);
		const RenderGraphImageHandle sceneColor = AddGeometryPass(renderExtent);

		//const FBlurGraphOutput blurOutput = AddBlurPass(sceneColor, renderExtent);

		AddToneMappingPass(sceneColor, renderExtent);

		renderGraph.Compile();
		toneMappingPass->SetSource(renderGraph.GetImageView(sceneColor));

		//blurPass->SetResources(
		//	renderGraph.GetImageView(sceneColor),
		//	renderGraph.GetImageView(blurOutput.temp),
		//	renderGraph.GetImageView(blurOutput.output));
	}

	RenderGraphImageHandle Renderer::AddGeometryPass(VkExtent2D renderExtent)
	{
		FVulkanImageDesc sceneColorDesc = {};
		sceneColorDesc.format = GeometryPass::ColorFormat;
		sceneColorDesc.extent = { renderExtent.width, renderExtent.height, 1 };
		sceneColorDesc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		FVulkanImageDesc sceneDepthDesc = {};
		sceneDepthDesc.format = GeometryPass::DepthFormat;
		sceneDepthDesc.extent = { renderExtent.width, renderExtent.height, 1 };
		sceneDepthDesc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		struct FGeometryPassData
		{
			RenderGraphImageHandle sceneColor;
			RenderGraphImageHandle sceneDepth;
		};

		const FGeometryPassData geometryPassData = renderGraph.AddPass<FGeometryPassData>(
			"GeometryPass",
			[sceneColorDesc, sceneDepthDesc](RenderGraph::Builder& builder, FGeometryPassData& data)
			{
				data.sceneColor = builder.DeclareInternal(sceneColorDesc);
				data.sceneDepth = builder.DeclareInternal(sceneDepthDesc);

				data.sceneColor = builder.Write(data.sceneColor, EImageState::ColorAttachmentWrite);
				data.sceneDepth = builder.Write(data.sceneDepth, EImageState::DepthStencilAttachmentWrite);
			},

			[this, renderExtent](VkCommandBuffer commandBuffer, const FGeometryPassData& data)
			{
				geometryPass->Execute(
					commandBuffer,
					renderGraph.GetImageView(data.sceneColor),
					renderGraph.GetImageView(data.sceneDepth),
					renderExtent,
					viewData);
			}
		);

		return geometryPassData.sceneColor;
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

	void Renderer::AddToneMappingPass(RenderGraphImageHandle input, VkExtent2D renderExtent)
	{
		FVulkanImageDesc backBufferDesc = {};
		backBufferDesc.format = VulkanSwapchain::swapchainFormat;
		backBufferDesc.extent = { renderExtent.width, renderExtent.height, 1 };
		backBufferDesc.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		struct FToneMappingPassData
		{
			RenderGraphImageHandle sceneColor;
			RenderGraphImageHandle backBuffer;
		};

		renderGraph.AddPass<FToneMappingPassData>(
			"ToneMappingPass",
			[this, input, backBufferDesc](RenderGraph::Builder& builder, FToneMappingPassData& data)
			{
				data.sceneColor = builder.Read(input, EImageState::FragmentSampledRead);
				data.backBuffer = builder.DeclareExternal(backBufferDesc, EImageState::Undefined, EImageState::Present);
				renderGraphBackBuffer = data.backBuffer;
				data.backBuffer = builder.Write(data.backBuffer, EImageState::ColorAttachmentWrite);
			},
			[this, renderExtent](VkCommandBuffer commandBuffer, const FToneMappingPassData& data)
			{
				toneMappingPass->Execute(commandBuffer, renderGraph.GetImageView(data.backBuffer), renderExtent);
			});
	}

	void Renderer::Shutdown()
	{
		geometryPass.reset();
		imagePass.reset();
		blurPass.reset();
		toneMappingPass.reset();

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

	void Renderer::Render(const FViewData& _viewData)
	{
		// 전달받은 카메라 데이터를 이 프레임의 복사본으로 보관한다.
		viewData = _viewData;

		const FFrameResource& frameResource = device.GetCurrentFrameResource();
		const VkCommandBuffer commandBuffer = frameResource.commandBuffer;

		GPUProfiler& profiler = device.GetGPUProfiler();
		GPUProfiler::Scope frameScope(profiler, "Render");

		renderGraph.BindExternal(renderGraphBackBuffer, device.GetCurrentSwapchainImage(), device.GetCurrentSwapchainImageView());
		renderGraph.Execute(commandBuffer);
	}
}
