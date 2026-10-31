#include "pch.h"
#include "Renderer.h"
#include "GPUProfiler.h"
#include "Core/Paths.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Graphics/Vulkan/VulkanImage.h"
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
	}

	void Renderer::BuildRenderGraph()
	{
		// BackBuffer
		// ImagePass -> BlurPass -> PresentPass

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

	void Renderer::TransitionPresentPassImages(VkCommandBuffer commandBuffer)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();

		VkImageMemoryBarrier2 barrier = {};
		 barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		 barrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		 barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		 barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		 barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		 barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		 barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		 barrier.image = computeOutputImages[frameResource.frameSlot]->GetImage();
		 barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		 barrier.subresourceRange.baseMipLevel = 0;
		 barrier.subresourceRange.levelCount = 1;
		 barrier.subresourceRange.baseArrayLayer = 0;
		 barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo depInfo = {};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);
	}

	void Renderer::CopyToPresentImage(VkCommandBuffer commandBuffer, VkImage finalImage)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();

		VkImage presentImage = device.GetCurrentSwapchainImage();

		VkImageMemoryBarrier2 barrier = {};
		
		// 스왑체인 이미지 배리어 
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.image = presentImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo depInfo = {};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		const int32 sourceWidth = static_cast<int32>(computeOutputImages[frameResource.frameSlot]->GetWidth());
		const int32 sourceHeight = static_cast<int32>(computeOutputImages[frameResource.frameSlot]->GetHeight());
		const int32 destinationWidth = static_cast<int32>(device.GetSwapchain().GetWidth());
		const int32 destinationHeight = static_cast<int32>(device.GetSwapchain().GetHeight());

		VkImageBlit blitRegion{};
		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[1] = { sourceWidth, sourceHeight, 1 };

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[1] = { destinationWidth, destinationHeight, 1 };

		vkCmdBlitImage(
			commandBuffer,
			finalImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			presentImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blitRegion,
			VK_FILTER_NEAREST);
	}

	void Renderer::Render()
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();
		const VkCommandBuffer commandBuffer = frameResource.commandBuffer;

		GPUProfiler& profiler = device.GetGPUProfiler();
		GPUProfiler::Scope frameScope(profiler, "Render");
		{
			GPUProfiler::Scope scope(profiler, "Image");
			VulkanImage& sceneColor = *sceneColorImages[frameResource.frameSlot];
			const VkExtent2D extent = { sceneColor.GetWidth(), sceneColor.GetHeight() };
			imagePass->Execute(commandBuffer, sceneColor.GetImageView(), extent);
		}
		{
			GPUProfiler::Scope scope(profiler, "Blur");
			blurPass->Execute(commandBuffer, frameResource.frameSlot, { device.GetSwapchain().GetWidth(), device.GetSwapchain().GetHeight() });
		}

		const VkImage finalImage = computeOutputImages[frameResource.frameSlot]->GetImage();
		GPUProfiler::Scope presentScope(profiler, "Blit");
		TransitionPresentPassImages(commandBuffer);
		CopyToPresentImage(commandBuffer, finalImage);
	}
}
