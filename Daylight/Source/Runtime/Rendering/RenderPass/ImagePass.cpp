#include "pch.h"
#include "ImagePass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Resources/MeshResource.h"
#include "Resources/ResourceManager.h"
#include "Resources/ShaderResource.h"
#include "Resources/TextureResource.h"

namespace Dlight
{
	ImagePass::ImagePass(VulkanDevice& _device, ResourceManager& _resourceManager, ShaderCompiler& _shaderCompiler)
		: device(_device), resourceManager(_resourceManager), shaderCompiler(_shaderCompiler)
	{
		CreateMesh();
		CreateDescriptors();
		CreatePipeline();
	}

	ImagePass::~ImagePass()
	{
		pipeline.reset();
		DestroyDescriptors();
	}

	void ImagePass::SetTexture(TextureResource& texture)
	{
		VkDescriptorImageInfo textureInfo = {};
		textureInfo.imageView = texture.GetImage()->GetImageView();
		textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo samplerInfo = {};
		samplerInfo.sampler = texture.GetSampler()->GetSampler();

		VkWriteDescriptorSet writes[2] = {};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSet;
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageInfo = &textureInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet;
		writes[1].dstBinding = 1;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pImageInfo = &samplerInfo;

		vkUpdateDescriptorSets(device.GetDevice(), 2, writes, 0, nullptr);
	}

	void ImagePass::Execute(VkCommandBuffer commandBuffer, VkImageView renderTargetView, VkExtent2D extent)
	{
		VkRenderingAttachmentInfo colorAttachmentInfo = {};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = renderTargetView;
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = { 0.22f, 0.22f, 0.22f, 1.f };

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		VkViewport viewport = {};
		viewport.y = static_cast<float>(extent.height);
		viewport.width = static_cast<float>(extent.width);
		viewport.height = -static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.extent = extent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetPipelineLayout(),
			0,
			1,
			&descriptorSet,
			0,
			nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

		const VkBuffer vertexBuffer = mesh->GetVertexBuffer()->GetBuffer();
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer()->GetBuffer(), 0, mesh->GetIndexType());
		vkCmdDrawIndexed(commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);

		vkCmdEndRendering(commandBuffer);
	}

	void ImagePass::CreateMesh()
	{
		FMeshRaw meshData = {};
		meshData.vertexData.resize(sizeof(vertices[0]) * vertices.size());
		std::memcpy(meshData.vertexData.data(), vertices.data(), meshData.vertexData.size());
		meshData.indices = indices;
		meshData.layout.stride = sizeof(VertexFormat::FTexturedVertex);
		meshData.layout.attributes.resize(2);
		meshData.layout.attributes[0].location = 0;
		meshData.layout.attributes[0].binding = 0;
		meshData.layout.attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		meshData.layout.attributes[0].offset = offsetof(VertexFormat::FTexturedVertex, position);
		meshData.layout.attributes[1].location = 1;
		meshData.layout.attributes[1].binding = 0;
		meshData.layout.attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
		meshData.layout.attributes[1].offset = offsetof(VertexFormat::FTexturedVertex, texcoord);

		mesh = std::make_unique<MeshResource>("ImageTest", device, std::move(meshData));
		if (!mesh->Load())
		{
			DL_LOG_ERROR("Failed to load image mesh");
			std::abort();
		}
	}

	void ImagePass::CreateDescriptors()
	{
		constexpr uint32 TextureCount = 64;

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[0].descriptorCount = TextureCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[1].descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		vkCreateDescriptorPool(device.GetDevice(), &poolInfo, nullptr, &descriptorPool);

		std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[0].descriptorCount = TextureCount;
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorBindingFlags, 2> bindingFlags{};
		bindingFlags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = static_cast<uint32>(bindingFlags.size());
		bindingFlagsInfo.pBindingFlags = bindingFlags.data();

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = &bindingFlagsInfo;
		layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorSetLayout;
		vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, &descriptorSet);
	}

	void ImagePass::CreatePipeline()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"ImageTest.hlsl");

		FShaderRaw vertexShaderRaw = {};
		vertexShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::VertexEntry, L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShader = std::make_unique<ShaderResource>("ImageTest.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw = {};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::PixelEntry, L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShader = std::make_unique<ShaderResource>("ImageTest.PS", device, std::move(fragmentShaderRaw));

		if (!vertexShader->Load() || !fragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load image shaders");
			std::abort();
		}

		FGraphicsPipelineDesc desc = {};
		desc.descriptorSetLayouts = { descriptorSetLayout };
		desc.vertexShader = vertexShader->GetModule();
		desc.fragmentShader = fragmentShader->GetModule();
		desc.vertexEntryPoint = vertexShader->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = fragmentShader->GetEntryPoint().c_str();

		VkVertexInputBindingDescription vertexBinding = {};
		vertexBinding.binding = 0;
		vertexBinding.stride = mesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);
		desc.vertexAttributes = mesh->GetVertexLayout().attributes;
		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = VK_FORMAT_UNDEFINED;

		pipeline = std::make_unique<VulkanGraphicsPipeline>(device, desc);
	}

	void ImagePass::DestroyDescriptors()
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
