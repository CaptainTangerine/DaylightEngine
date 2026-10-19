#include "pch.h"
#include "Renderer.h"

#include "Core/Paths.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "ShaderConventions.h"
#include "Resources/MeshResource.h"
#include "Resources/TextureResource.h"
#include "Resources/ShaderResource.h"

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
		const std::filesystem::path imagePath = Paths::GetAssetPath(L"Image/Test.jpg");
		FTextureRaw textureData = { Image(imagePath.string()), {} };
		testTexture = std::make_unique<TextureResource>("ImageTest", device, std::move(textureData));
		if (!testTexture->Load())
		{
			DL_LOG_ERROR("Failed to load test texture");
			std::abort();
		}

		FMeshRaw imageMeshData = {};
		imageMeshData.vertexData.resize(sizeof(imageVertices[0]) * imageVertices.size());
		std::memcpy(imageMeshData.vertexData.data(), imageVertices.data(), imageMeshData.vertexData.size());
		imageMeshData.indices = imageIndices;
		imageMeshData.layout.stride = sizeof(VertexFormat::FTexturedVertex);
		imageMeshData.layout.attributes.resize(2);
		imageMeshData.layout.attributes[0].location = 0;
		imageMeshData.layout.attributes[0].binding = 0;
		imageMeshData.layout.attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		imageMeshData.layout.attributes[0].offset = offsetof(VertexFormat::FTexturedVertex, position);
		imageMeshData.layout.attributes[1].location = 1;
		imageMeshData.layout.attributes[1].binding = 0;
		imageMeshData.layout.attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
		imageMeshData.layout.attributes[1].offset = offsetof(VertexFormat::FTexturedVertex, texcoord);

		imageMesh = std::make_unique<MeshResource>("ImageTest", device, std::move(imageMeshData));
		if (!imageMesh->Load())
		{
			DL_LOG_ERROR("Failed to load image mesh");
			std::abort();
		}

		FMeshRaw triangleMeshData = {};
		triangleMeshData.vertexData.resize(sizeof(triangleVertices[0]) * triangleVertices.size());
		std::memcpy(triangleMeshData.vertexData.data(), triangleVertices.data(), triangleMeshData.vertexData.size());
		triangleMeshData.indices = triangleIndices;
		triangleMeshData.layout.stride = sizeof(VertexFormat::FColoredVertex);
		triangleMeshData.layout.attributes.resize(2);
		triangleMeshData.layout.attributes[0].location = 0;
		triangleMeshData.layout.attributes[0].binding = 0;
		triangleMeshData.layout.attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		triangleMeshData.layout.attributes[0].offset = offsetof(VertexFormat::FColoredVertex, position);
		triangleMeshData.layout.attributes[1].location = 1;
		triangleMeshData.layout.attributes[1].binding = 0;
		triangleMeshData.layout.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		triangleMeshData.layout.attributes[1].offset = offsetof(VertexFormat::FColoredVertex, color);

		triangleMesh = std::make_unique<MeshResource>("Triangle", device, std::move(triangleMeshData));
		if (!triangleMesh->Load())
		{
			DL_LOG_ERROR("Failed to load triangle mesh");
			std::abort();
		}
		
		VkDescriptorImageInfo textureInfo = {};
		textureInfo.imageView = testTexture->GetImage()->GetImageView();
		textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		VkDescriptorImageInfo samplerDescriptorInfo = {};
		samplerDescriptorInfo.sampler = testTexture->GetSampler()->GetSampler();

		VkWriteDescriptorSet writes[2] = {};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = device.GetBindlessSet();
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageInfo = &textureInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = device.GetSamplerSet();
		writes[1].dstBinding = 0;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pImageInfo = &samplerDescriptorInfo;

		vkUpdateDescriptorSets(device.GetDevice(), 2, writes, 0, nullptr);

		CreateTrianglePipelines();
		CreateImageTestPipelines();
	}

	void Renderer::Shutdown()
	{
		DestroyPipelines();

		triangleVertexShader.reset();
		triangleFragmentShader.reset();
		imageVertexShader.reset();
		imageFragmentShader.reset();

		testTexture.reset();
	}

	bool Renderer::CreateTrianglePipelines()
	{
		// 셰이더 컴파일 -> 추후 shaderManager로 관리 
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Triangle.hlsl");

		if (!triangleVertexShader)
		{
			FShaderRaw shaderRaw = {};
			shaderRaw.spirv = shaderCompiler.Compile(
				shaderPath.c_str(),
				ShaderConventions::VertexEntry,
				L"vs_6_0");
			shaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderRaw.entryPoint = ShaderConventions::VertexEntry;

			triangleVertexShader = std::make_unique<ShaderResource>(
				"Triangle.VS", device, std::move(shaderRaw));
		}

		if (!triangleFragmentShader)
		{
			FShaderRaw shaderRaw = {};
			shaderRaw.spirv = shaderCompiler.Compile(
				shaderPath.c_str(),
				ShaderConventions::PixelEntry,
				L"ps_6_0");
			shaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderRaw.entryPoint = ShaderConventions::PixelEntry;

			triangleFragmentShader = std::make_unique<ShaderResource>(
				"Triangle.PS", device, std::move(shaderRaw));
		}

		if (!triangleVertexShader->Load() || !triangleFragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load triangle shaders");
			std::abort();
		}

		FGfxPipelineDesc desc{};
		desc.vertexShader = triangleVertexShader->GetModule();
		desc.fragmentShader = triangleFragmentShader->GetModule();
		desc.vertexEntryPoint = triangleVertexShader->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = triangleFragmentShader->GetEntryPoint().c_str();


		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = triangleMesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);

		// dx11 InputLayout 같은 개념 
		desc.vertexAttributes = triangleMesh->GetVertexLayout().attributes;

		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = device.GetDepthStencilFormat();

		trianglePipeline = std::make_unique<VulkanPipeline>(device, desc);

		return true;
	}

	bool Renderer::CreateImageTestPipelines()
	{
		// 셰이더 컴파일 -> 추후 shaderManager로 관리 
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"ImageTest.hlsl");

		if (!imageVertexShader)
		{
			FShaderRaw shaderRaw = {};
			shaderRaw.spirv = shaderCompiler.Compile(
				shaderPath.c_str(),
				ShaderConventions::VertexEntry,
				L"vs_6_0");
			shaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderRaw.entryPoint = ShaderConventions::VertexEntry;

			imageVertexShader = std::make_unique<ShaderResource>(
				"ImageTest.VS", device, std::move(shaderRaw));
		}

		if (!imageFragmentShader)
		{
			FShaderRaw shaderRaw = {};
			shaderRaw.spirv = shaderCompiler.Compile(
				shaderPath.c_str(),
				ShaderConventions::PixelEntry,
				L"ps_6_0");
			shaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderRaw.entryPoint = ShaderConventions::PixelEntry;

			imageFragmentShader = std::make_unique<ShaderResource>(
				"ImageTest.PS", device, std::move(shaderRaw));
		}

		if (!imageVertexShader->Load() || !imageFragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load image shaders");
			std::abort();
		}

		FGfxPipelineDesc desc{};
		desc.descriptorSetLayouts =
		{
			device.GetBindlessLayout(),
			device.GetSamplerLayout()
		};

		desc.vertexShader = imageVertexShader->GetModule();
		desc.fragmentShader = imageFragmentShader->GetModule();
		desc.vertexEntryPoint = imageVertexShader->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = imageFragmentShader->GetEntryPoint().c_str();

		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = imageMesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);

		// dx11 InputLayout 같은 개념 
		desc.vertexAttributes = imageMesh->GetVertexLayout().attributes;

		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = device.GetDepthStencilFormat();

		imageTestPipeline = std::make_unique<VulkanPipeline>(device, desc);

		return true;
	}

	void Renderer::DestroyPipelines()
	{
		if (trianglePipeline)
		{
			trianglePipeline.reset();
		}

		if (imageTestPipeline)
		{
			imageTestPipeline.reset();
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

			VkDescriptorSet sets[2] =
			{
				device.GetBindlessSet(),
				device.GetSamplerSet()
			};

			vkCmdBindDescriptorSets(
				frameCtx.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				imageTestPipeline->GetPipelineLayout(),
				0,
				2,
				sets,
				0,
				nullptr);
			
			//vkCmdBindPipeline(frameCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline->GetPipeline());
			vkCmdBindPipeline(frameCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, imageTestPipeline->GetPipeline());
			

			const VkBuffer vertexBuffer = imageMesh->GetVertexBuffer()->GetBuffer();
			const VkDeviceSize offset = 0;
			
			vkCmdBindVertexBuffers(frameCtx.commandBuffer, 0, 1, &vertexBuffer, &offset);
			vkCmdBindIndexBuffer(frameCtx.commandBuffer, imageMesh->GetIndexBuffer()->GetBuffer(), 0, imageMesh->GetIndexType());
			vkCmdDrawIndexed(frameCtx.commandBuffer, imageMesh->GetIndexCount(), 1, 0, 0, 0);


		}
		vkCmdEndRendering(frameCtx.commandBuffer);
	}

}
