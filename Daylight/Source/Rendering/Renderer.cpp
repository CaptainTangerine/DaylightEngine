#include "pch.h"
#include "Renderer.h"

#include "Core/Paths.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Graphics/Vulkan/VulkanPipeline.h"

#include "ShaderConventions.h"

namespace Dlight
{
	Renderer::Renderer(VulkanDevice& _device)
		: device(_device)
	{
		Initialize();
	}

	Renderer::~Renderer()
	{
		Shutdown();
	}

	void Renderer::Initialize()
	{
		CreatePipelines();
	}

	void Renderer::Shutdown()
	{
		DestroyPipelines();
	}

	bool Renderer::CreatePipelines()
	{
		// 셰이더 컴파일 -> 추후 shaderManager로 관리 
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Triangle.hlsl");

		auto vertCode = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::VertexEntry,
			L"vs_6_0");

		auto fragCode = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::PixelEntry,
			L"ps_6_0");

		// 셰이더 모듈 만들기 
		VkShaderModule vertModule = VK_NULL_HANDLE;
		VkShaderModule fragModule = VK_NULL_HANDLE;

		VkShaderModuleCreateInfo vertModuleInfo{};
		vertModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertModuleInfo.codeSize = vertCode.size() * sizeof(uint32);
		vertModuleInfo.pCode = vertCode.data();
		if (VK_SUCCESS != vkCreateShaderModule(device.GetDevice(), &vertModuleInfo, nullptr, &vertModule))
		{
			DL_LOG_ERROR("Failed to create vertex shader module");
			std::abort();
		}

		VkShaderModuleCreateInfo fragModuleInfo{};
		fragModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragModuleInfo.codeSize = fragCode.size() * sizeof(uint32);
		fragModuleInfo.pCode = fragCode.data();
		if (VK_SUCCESS != vkCreateShaderModule(device.GetDevice(), &fragModuleInfo, nullptr, &fragModule))
		{
			DL_LOG_ERROR("Failed to create fragment shader module");
			std::abort();
		}


		GfxPipelineDesc desc{};
		desc.vertexShader = vertModule;
		desc.fragmentShader = fragModule;
		desc.vertexEntryPoint = ShaderConventions::VertexEntry;
		desc.fragmentEntryPoint = ShaderConventions::PixelEntry;
		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = device.GetDepthStencilFormat();

		trianglePipeline = std::make_unique<VulkanPipeline>(device, desc);

		vkDestroyShaderModule(device.GetDevice(), vertModule, nullptr);
		vkDestroyShaderModule(device.GetDevice(), fragModule, nullptr);

		return true;
	}

	void Renderer::DestroyPipelines()
	{
		if (trianglePipeline)
		{
			trianglePipeline.reset();
		}
	}

	void Renderer::Render()
	{
		const auto frameCtx = device.GetCurrentRenderFrameContext();

		VkRenderingAttachmentInfo colorAttachmentInfo = {};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = frameCtx.colorImageView;
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = { 0.22f, 0.22f, 0.22f, 1.f };

		VkRenderingAttachmentInfo depthAttachmentInfo = {};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = frameCtx.depthStencilImageView;
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentInfo.clearValue.depthStencil = {1.f, 0};

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset.x = 0;
		renderingInfo.renderArea.offset.y = 0;
		renderingInfo.renderArea.extent = frameCtx.extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		vkCmdBeginRendering(frameCtx.commandBuffer, &renderingInfo);

		{
			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = static_cast<float>(frameCtx.extent.height);
			viewport.width = static_cast<float>(frameCtx.extent.width);
			// 다렉 좌표계를 따르고 마지막 뷰포트시 음수로 하여서 vulkan 좌표계와 일치
			viewport.height = -static_cast<float>(frameCtx.extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(frameCtx.commandBuffer, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent = frameCtx.extent;
			vkCmdSetScissor(frameCtx.commandBuffer, 0, 1, &scissor);

			// draw triangle !!!!!!!
			vkCmdBindPipeline(frameCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline->GetPipeline());
			vkCmdDraw(frameCtx.commandBuffer, 3, 1, 0, 0);
		}
		vkCmdEndRendering(frameCtx.commandBuffer);
	}

}
