#include "pch.h"
#include "SkyPass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Resources/ShaderResource.h"

namespace Dlight
{
	SkyPass::SkyPass(VulkanDevice& _device, ShaderCompiler& _shaderCompiler)
		: device(_device)
		, shaderCompiler(_shaderCompiler)
	{
		CreateDescriptors();
		CreatePipeline();
	}

	SkyPass::~SkyPass()
	{
		pipeline.reset();
		fragmentShader.reset();
		vertexShader.reset();
		sampler.reset();
		DestroyDescriptors();
	}

	void SkyPass::SetCubemap(const VulkanImage& cubemap)
	{
		VkDescriptorImageInfo cubemapInfo{};
		cubemapInfo.imageView = cubemap.GetImageView();
		cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo samplerInfo{};
		samplerInfo.sampler = sampler->GetSampler();

		for (VkDescriptorSet descriptorSet : descriptorSets)
		{
			std::array<VkWriteDescriptorSet, 2> writes{};
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = descriptorSet;
			writes[0].dstBinding = 1;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[0].pImageInfo = &cubemapInfo;
			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = descriptorSet;
			writes[1].dstBinding = 2;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			writes[1].pImageInfo = &samplerInfo;

			vkUpdateDescriptorSets(
				device.GetDevice(),
				static_cast<uint32>(writes.size()),
				writes.data(),
				0,
				nullptr);
		}
	}

	void SkyPass::Execute(VkCommandBuffer commandBuffer, VkImageView sceneColorView, VkExtent2D extent, const FViewData& viewData)
	{
		UpdateViewData(viewData);

		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = sceneColorView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		VkViewport viewport{};
		viewport.y = static_cast<float>(extent.height);
		viewport.width = static_cast<float>(extent.width);
		viewport.height = -static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = extent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		const uint32 frameSlot = device.GetCurrentFrameResource().frameSlot;
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetPipelineLayout(),
			0,
			1,
			&descriptorSets[frameSlot],
			0,
			nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRendering(commandBuffer);
	}

	void SkyPass::CreateDescriptors()
	{
		std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[2].binding = 2;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[2].descriptorCount = 1;
		bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		if (VK_SUCCESS != vkCreateDescriptorSetLayout(
			device.GetDevice(),
			&layoutInfo,
			nullptr,
			&descriptorSetLayout))
		{
			DL_LOG_ERROR("Failed to create sky descriptor set layout");
			std::abort();
		}

		constexpr uint32 frameCount = VulkanDevice::MaxFramesInFlight;
		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = frameCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[1].descriptorCount = frameCount;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[2].descriptorCount = frameCount;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = frameCount;
		poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		if (VK_SUCCESS != vkCreateDescriptorPool(
			device.GetDevice(),
			&poolInfo,
			nullptr,
			&descriptorPool))
		{
			DL_LOG_ERROR("Failed to create sky descriptor pool");
			std::abort();
		}

		std::array<VkDescriptorSetLayout, frameCount> layouts{};
		layouts.fill(descriptorSetLayout);
		descriptorSets.resize(frameCount);

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = frameCount;
		allocateInfo.pSetLayouts = layouts.data();
		if (VK_SUCCESS != vkAllocateDescriptorSets(
			device.GetDevice(),
			&allocateInfo,
			descriptorSets.data()))
		{
			DL_LOG_ERROR("Failed to allocate sky descriptor sets");
			std::abort();
		}

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		sampler = std::make_unique<VulkanSampler>(device, samplerInfo);

		viewUniformBuffers.reserve(frameCount);
		for (uint32 frameSlot = 0; frameSlot < frameCount; ++frameSlot)
		{
			FVulkanBufferDesc bufferDesc{};
			bufferDesc.size = sizeof(FPerViewUniform);
			bufferDesc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferDesc.memoryUsage = VMA_MEMORY_USAGE_AUTO;
			bufferDesc.allocationFlags =
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;

			auto viewUniformBuffer = std::make_unique<VulkanBuffer>(device, bufferDesc);

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = viewUniformBuffer->GetBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(FPerViewUniform);

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = descriptorSets[frameSlot];
			write.dstBinding = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(device.GetDevice(), 1, &write, 0, nullptr);

			viewUniformBuffers.push_back(std::move(viewUniformBuffer));
		}
	}

	void SkyPass::DestroyDescriptors()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device.GetDevice(), descriptorPool, nullptr);
			descriptorPool = VK_NULL_HANDLE;
		}

		descriptorSets.clear();

		if (descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device.GetDevice(), descriptorSetLayout, nullptr);
			descriptorSetLayout = VK_NULL_HANDLE;
		}

		viewUniformBuffers.clear();
	}

	void SkyPass::CreatePipeline()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Sky.hlsl");

		FShaderRaw vertexShaderRaw{};
		vertexShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::VertexEntry,
			L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShader = std::make_unique<ShaderResource>("Sky.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw{};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::PixelEntry,
			L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShader = std::make_unique<ShaderResource>("Sky.PS", device, std::move(fragmentShaderRaw));

		if (!vertexShader->Load() || !fragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load sky shaders");
			std::abort();
		}

		FGraphicsPipelineDesc pipelineDesc{};
		pipelineDesc.vertexShader = vertexShader->GetModule();
		pipelineDesc.fragmentShader = fragmentShader->GetModule();
		pipelineDesc.vertexEntryPoint = vertexShader->GetEntryPoint().c_str();
		pipelineDesc.fragmentEntryPoint = fragmentShader->GetEntryPoint().c_str();
		pipelineDesc.descriptorSetLayouts = { descriptorSetLayout };
		pipelineDesc.cullMode = VK_CULL_MODE_NONE;
		pipelineDesc.colorFormat = ColorFormat;
		pipelineDesc.depthFormat = VK_FORMAT_UNDEFINED;
		pipeline = std::make_unique<VulkanGraphicsPipeline>(device, pipelineDesc);
	}

	void SkyPass::UpdateViewData(const FViewData& viewData)
	{
		const FPerViewUniform uniformData =
		{
			viewData.viewMatrix,
			viewData.projectionMatrix,
			viewData.viewMatrix.Invert(),
			viewData.projectionMatrix.Invert(),
			viewData.cameraPosition
		};

		const uint32 frameSlot = device.GetCurrentFrameResource().frameSlot;
		VulkanBuffer& viewBuffer = *viewUniformBuffers[frameSlot];
		std::memcpy(viewBuffer.GetMappedData(), &uniformData, sizeof(uniformData));
		viewBuffer.FlushAllocation(0, sizeof(uniformData));
	}
}
