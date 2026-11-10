#include "pch.h"
#include "ToneMappingPass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Rendering/PushConstants.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Resources/ShaderResource.h"

namespace Dlight
{
	ToneMappingPass::ToneMappingPass(VulkanDevice& _device, ShaderCompiler& _shaderCompiler)
		: device(_device), shaderCompiler(_shaderCompiler)
	{
		CreateDescriptors();
		CreatePipeline();
	}

	ToneMappingPass::~ToneMappingPass()
	{
		pipeline.reset();
		fragmentShader.reset();
		vertexShader.reset();
		sampler.reset();
		DestroyDescriptors();
	}

	void ToneMappingPass::SetSource(VkImageView sourceView)
	{
		VkDescriptorImageInfo sourceInfo{};
		sourceInfo.imageView = sourceView;
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo samplerInfo{};
		samplerInfo.sampler = sampler->GetSampler();

		std::array<VkWriteDescriptorSet, 2> writes{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSet;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageInfo = &sourceInfo;
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet;
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pImageInfo = &samplerInfo;

		vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
	}

	void ToneMappingPass::Execute(VkCommandBuffer commandBuffer, VkImageView renderTargetView, VkExtent2D extent)
	{
		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = renderTargetView;
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
		FToneMappingPushConstants constants{ 3.f };
		vkCmdPushConstants(commandBuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), &constants);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRendering(commandBuffer);
	}

	void ToneMappingPass::CreateDescriptors()
	{
		std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		if (VK_SUCCESS != vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout))
		{
			DL_LOG_ERROR("Failed to create tone mapping descriptor set layout");
			std::abort();
		}

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[0].descriptorCount = 1;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[1].descriptorCount = 1;
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		if (VK_SUCCESS != vkCreateDescriptorPool(device.GetDevice(), &poolInfo, nullptr, &descriptorPool))
		{
			DL_LOG_ERROR("Failed to create tone mapping descriptor pool");
			std::abort();
		}

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorSetLayout;
		if (VK_SUCCESS != vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, &descriptorSet))
		{
			DL_LOG_ERROR("Failed to allocate tone mapping descriptor set");
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
		samplerInfo.maxLod = 0.0f;
		sampler = std::make_unique<VulkanSampler>(device, samplerInfo);
	}

	void ToneMappingPass::CreatePipeline()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"ToneMapping.hlsl");

		FShaderRaw vertexShaderRaw{};
		vertexShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::VertexEntry, L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShader = std::make_unique<ShaderResource>("ToneMapping.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw{};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::PixelEntry, L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShader = std::make_unique<ShaderResource>("ToneMapping.PS", device, std::move(fragmentShaderRaw));
		if (!vertexShader->Load() || !fragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load tone mapping shaders");
			std::abort();
		}

		FGraphicsPipelineDesc desc{};
		desc.vertexShader = vertexShader->GetModule();
		desc.fragmentShader = fragmentShader->GetModule();
		desc.vertexEntryPoint = vertexShader->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = fragmentShader->GetEntryPoint().c_str();
		desc.descriptorSetLayouts = { descriptorSetLayout };
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.size = sizeof(FToneMappingPushConstants);
		desc.pushConstantRanges = { pushConstantRange };
		desc.cullMode = VK_CULL_MODE_NONE;
		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		pipeline = std::make_unique<VulkanGraphicsPipeline>(device, desc);
	}

	void ToneMappingPass::DestroyDescriptors()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device.GetDevice(), descriptorPool, nullptr);
			descriptorPool = VK_NULL_HANDLE;
		}
		descriptorSet = VK_NULL_HANDLE;
		if (descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device.GetDevice(), descriptorSetLayout, nullptr);
			descriptorSetLayout = VK_NULL_HANDLE;
		}
	}
}
