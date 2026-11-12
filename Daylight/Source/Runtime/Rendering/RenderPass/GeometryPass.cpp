#include "pch.h"
#include "GeometryPass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Rendering/RenderPass/SphereMeshData.h"
#include "Resources/MeshResource.h"
#include "Resources/ResourceManager.h"
#include "Resources/ShaderResource.h"
#include "Resources/TextureResource.h"
#include "Rendering/PushConstants.h"

namespace Dlight
{
	GeometryPass::GeometryPass(VulkanDevice& _device, ResourceManager& _resourceManager, ShaderCompiler& _shaderCompiler)
		: device(_device), resourceManager(_resourceManager), shaderCompiler(_shaderCompiler)
	{
		CreateMesh();
		CreateImages();
		CreateUniformResources();
		CreatePipelineUnLit();
		CreatePipelinePBR();
	}

	GeometryPass::~GeometryPass()
	{
		DestroyUniformResources();
	}

	void GeometryPass::Execute(
		VkCommandBuffer commandBuffer,
		VkImageView sceneColorView,
		VkImageView sceneDepthView,
		VkExtent2D extent,
		const FViewData& _viewData)
	{
		UpdateViewData(_viewData);

		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = sceneColorView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = { 0.22f, 0.22f, 0.22f, 1.f };

		VkRenderingAttachmentInfo depthAttachment{};
		depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachment.imageView = sceneDepthView;
		depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = &depthAttachment;
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

		FPBRMaterial material = { { 1.f, 1.f, 1.f }, 1.f, 1.f, 1.f };
		vkCmdPushConstants(commandBuffer, pipelinePBR->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FPBRMaterial), &material);

		const uint32 frameSlot = device.GetCurrentFrameResource().frameSlot;
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelinePBR->GetPipelineLayout(),
			0, 1, &descriptorSets[frameSlot], 0, nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinePBR->GetPipeline());

		const VkBuffer vertexBuffer = mesh->GetVertexBuffer()->GetBuffer();
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer()->GetBuffer(), 0, mesh->GetIndexType());
		vkCmdDrawIndexed(commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);

		vkCmdEndRendering(commandBuffer);
	}

	void GeometryPass::CreateUniformResources()
	{
		std::array<VkDescriptorSetLayoutBinding, MaterialSamplerBinding + 1> bindings = {};
		
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		for (uint32 textureIndex = 0; textureIndex < MaterialTextureCount; ++textureIndex)
		{
			VkDescriptorSetLayoutBinding& textureBinding = bindings[MaterialTextureBinding + textureIndex];
			textureBinding.binding = MaterialTextureBinding + textureIndex;
			textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			textureBinding.descriptorCount = 1;
			textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		bindings[MaterialSamplerBinding].binding = MaterialSamplerBinding;
		bindings[MaterialSamplerBinding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[MaterialSamplerBinding].descriptorCount = 1;
		bindings[MaterialSamplerBinding].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (VK_SUCCESS != vkCreateDescriptorSetLayout(
			device.GetDevice(),
			&layoutInfo,
			nullptr,
			&descriptorSetLayout))
		{
			DL_LOG_ERROR("Failed to create geometry view descriptor set layout");
			std::abort();
		}

		constexpr uint32 frameCount = VulkanDevice::MaxFramesInFlight;

		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = frameCount * 2;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[1].descriptorCount = frameCount * MaterialTextureCount;
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
			DL_LOG_ERROR("Failed to create geometry view descriptor pool");
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
			DL_LOG_ERROR("Failed to allocate geometry view descriptor sets");
			std::abort();
		}

		viewUniformBuffers.reserve(frameCount);
		lightUniformBuffers.reserve(frameCount);

		for (uint32 frameSlot = 0; frameSlot < frameCount; ++frameSlot)
		{
			FVulkanBufferDesc viewBufferDesc{};
			viewBufferDesc.size = sizeof(FPerViewUniform);
			viewBufferDesc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			viewBufferDesc.memoryUsage = VMA_MEMORY_USAGE_AUTO;
			viewBufferDesc.allocationFlags =
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;

			auto viewUniformBuffer = std::make_unique<VulkanBuffer>(device, viewBufferDesc);

			FVulkanBufferDesc lightBufferDesc{};
			lightBufferDesc.size = sizeof(FLightUniform);
			lightBufferDesc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			lightBufferDesc.memoryUsage = VMA_MEMORY_USAGE_AUTO;
			lightBufferDesc.allocationFlags = 
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;

			auto lightUniformBuffer = std::make_unique<VulkanBuffer>(device, lightBufferDesc);

			VkDescriptorBufferInfo viewBufferInfo{};
			viewBufferInfo.buffer = viewUniformBuffer->GetBuffer();
			viewBufferInfo.offset = 0;
			viewBufferInfo.range = sizeof(FPerViewUniform);

			VkDescriptorBufferInfo lightBufferInfo{};
			lightBufferInfo.buffer = lightUniformBuffer->GetBuffer();
			lightBufferInfo.offset = 0;
			lightBufferInfo.range = sizeof(FLightUniform);

			const std::array<const TextureResource*, MaterialTextureCount> materialTextures = { albedoMap.get(), normalMap.get(), heightMap.get(), metallicMap.get(), roughnessMap.get(), ambientOcclusionMap.get() };

			std::array<VkDescriptorImageInfo, MaterialTextureCount> textureInfos{};
			for (uint32 textureIndex = 0; textureIndex < MaterialTextureCount; ++textureIndex)
			{
				const TextureResource* textureResource = materialTextures[textureIndex];
				textureInfos[textureIndex].imageView = textureResource->GetImage()->GetImageView();
				textureInfos[textureIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			VkDescriptorImageInfo samplerInfo{};
			samplerInfo.sampler = albedoMap->GetSampler()->GetSampler();

			std::array<VkWriteDescriptorSet, MaterialSamplerBinding + 1> writes{};

			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = descriptorSets[frameSlot];
			writes[0].dstBinding = 0;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[0].pBufferInfo = &viewBufferInfo;

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = descriptorSets[frameSlot];
			writes[1].dstBinding = 1;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[1].pBufferInfo = &lightBufferInfo;

			for (uint32 textureIndex = 0; textureIndex < MaterialTextureCount; ++textureIndex)
			{
				VkWriteDescriptorSet& textureWrite = writes[MaterialTextureBinding + textureIndex];
				textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				textureWrite.dstSet = descriptorSets[frameSlot];
				textureWrite.dstBinding = MaterialTextureBinding + textureIndex;
				textureWrite.descriptorCount = 1;
				textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				textureWrite.pImageInfo = &textureInfos[textureIndex];
			}

			writes[MaterialSamplerBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[MaterialSamplerBinding].dstSet = descriptorSets[frameSlot];
			writes[MaterialSamplerBinding].dstBinding = MaterialSamplerBinding;
			writes[MaterialSamplerBinding].descriptorCount = 1;
			writes[MaterialSamplerBinding].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			writes[MaterialSamplerBinding].pImageInfo = &samplerInfo;

			vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
			viewUniformBuffers.push_back(std::move(viewUniformBuffer));
			lightUniformBuffers.push_back(std::move(lightUniformBuffer));
		}

		// Init Light Uniform
		const FLightUniform lightUniform =
		{
			{
				Vector4(0.0f, 5.0f, 5.0f, 1.0f),
				Vector4(-5.0f, 0.0f, 0.0f, 1.0f),
				Vector4(5.0f, 0.0f, 0.0f, 1.0f),
				Vector4(0.0f, -5.0f, 0.0f, 1.0f)
			},
			{
				Vector4(23.47f, 21.31f, 20.79f, 1.0f),
				Vector4(23.47f, 21.31f, 20.79f, 1.0f),
				Vector4(23.47f, 21.31f, 20.79f, 1.0f),
				Vector4(23.47f, 21.31f, 20.79f, 1.0f)
			}
		};

		for (const std::unique_ptr<VulkanBuffer>& lightUniformBuffer : lightUniformBuffers)
		{
			std::memcpy(
				lightUniformBuffer->GetMappedData(),
				&lightUniform,
				sizeof(lightUniform));
			lightUniformBuffer->FlushAllocation(0, sizeof(lightUniform));
		}
	}


	void GeometryPass::DestroyUniformResources()
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
		lightUniformBuffers.clear();
	}

	void GeometryPass::CreateMesh()
	{
		const SphereMeshData::FSphereMeshData sphere = SphereMeshData::Create();

		FMeshRaw meshData = {};
		meshData.vertexData.resize(sizeof(sphere.vertices[0]) * sphere.vertices.size());
		std::memcpy(meshData.vertexData.data(), sphere.vertices.data(), meshData.vertexData.size());
		meshData.indices = sphere.indices;
		meshData.layout.stride = sizeof(VertexFormat::FPBRVertex);
		meshData.layout.attributes.resize(5);
		meshData.layout.attributes[0].location = 0;
		meshData.layout.attributes[0].binding = 0;
		meshData.layout.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		meshData.layout.attributes[0].offset = offsetof(VertexFormat::FPBRVertex, position);
		meshData.layout.attributes[1].location = 1;
		meshData.layout.attributes[1].binding = 0;
		meshData.layout.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		meshData.layout.attributes[1].offset = offsetof(VertexFormat::FPBRVertex, color);
		meshData.layout.attributes[2].location = 2;
		meshData.layout.attributes[2].binding = 0;
		meshData.layout.attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		meshData.layout.attributes[2].offset = offsetof(VertexFormat::FPBRVertex, normal);
		meshData.layout.attributes[3].location = 3;
		meshData.layout.attributes[3].binding = 0;
		meshData.layout.attributes[3].format = VK_FORMAT_R32G32_SFLOAT;
		meshData.layout.attributes[3].offset = offsetof(VertexFormat::FPBRVertex, texcoord);
		meshData.layout.attributes[4].location = 4;
		meshData.layout.attributes[4].binding = 0;
		meshData.layout.attributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		meshData.layout.attributes[4].offset = offsetof(VertexFormat::FPBRVertex, tangent);

		mesh = std::make_unique<MeshResource>("Sphere", device, std::move(meshData));
		if (!mesh->Load())
		{
			DL_LOG_ERROR("Failed to load sphere mesh");
			std::abort();
		}
	}

	void GeometryPass::CreateImages()
	{
		albedoMap = std::make_unique<TextureResource>("GoldNugget.Albedo", device, Paths::GetAssetPath(L"Model/gold-nugget-ue/gold-nugget1_albedo.png"), ETextureSemantic::BaseColor);
		normalMap = std::make_unique<TextureResource>("GoldNugget.Normal", device, Paths::GetAssetPath(L"Model/gold-nugget-ue/gold-nugget1_normal-dx.png"), ETextureSemantic::Normal);
		heightMap = std::make_unique<TextureResource>("GoldNugget.Height", device, Paths::GetAssetPath(L"Model/gold-nugget-ue/gold-nugget1_height.png"), ETextureSemantic::Height);
		metallicMap = std::make_unique<TextureResource>("GoldNugget.Metallic", device, Paths::GetAssetPath(L"Model/gold-nugget-ue/gold-nugget1_metallic.png"), ETextureSemantic::Metallic);
		roughnessMap = std::make_unique<TextureResource>("GoldNugget.Roughness", device, Paths::GetAssetPath(L"Model/gold-nugget-ue/gold-nugget1_roughness.png"), ETextureSemantic::Roughness);
		ambientOcclusionMap = std::make_unique<TextureResource>("GoldNugget.AmbientOcclusion", device, Paths::GetAssetPath(L"Model/gold-nugget-ue/gold-nugget1_ao.png"), ETextureSemantic::AmbientOcclusion);

		const std::array<TextureResource*, MaterialTextureCount> materialTextures = { albedoMap.get(), normalMap.get(), heightMap.get(), metallicMap.get(), roughnessMap.get(), ambientOcclusionMap.get() };

		for (TextureResource* texture : materialTextures)
		{
			if (!texture->Load())
			{
				DL_LOG_ERROR("Failed to load material texture resource: ", texture->GetID());
				std::abort();
			}
		}
	}

	void GeometryPass::UpdateViewData(const FViewData& viewData)
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

	void GeometryPass::CreatePipelineUnLit()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Unlit.hlsl");

		FShaderRaw vertexShaderRaw = {};
		vertexShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::VertexEntry, L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShaderUnLit = std::make_unique<ShaderResource>("Unlit.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw = {};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::PixelEntry, L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShaderUnLit = std::make_unique<ShaderResource>("Unlit.PS", device, std::move(fragmentShaderRaw));

		if (!vertexShaderUnLit->Load() || !fragmentShaderUnLit->Load())
		{
			DL_LOG_ERROR("Failed to load unlit shaders");
			std::abort();
		}

		FGraphicsPipelineDesc desc{};
		desc.vertexShader = vertexShaderUnLit->GetModule();
		desc.fragmentShader = fragmentShaderUnLit->GetModule();
		desc.vertexEntryPoint = vertexShaderUnLit->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = fragmentShaderUnLit->GetEntryPoint().c_str();
		desc.descriptorSetLayouts = { descriptorSetLayout };

		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = mesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);
		desc.vertexAttributes = { mesh->GetVertexLayout().attributes[0], mesh->GetVertexLayout().attributes[1] };

		desc.colorFormat = ColorFormat;
		desc.depthFormat = DepthFormat;

		pipelineUnLit = std::make_unique<VulkanGraphicsPipeline>(device, desc);
	}

	void GeometryPass::CreatePipelinePBR()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"PBR.hlsl");

		FShaderRaw vertexShaderRaw{};
		vertexShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(), ShaderConventions::VertexEntry, L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShaderPBR = std::make_unique<ShaderResource>(
			"PBR.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw{};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(), ShaderConventions::PixelEntry, L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShaderPBR = std::make_unique<ShaderResource>(
			"PBR.PS", device, std::move(fragmentShaderRaw));

		if (!vertexShaderPBR->Load() || !fragmentShaderPBR->Load())
		{
			DL_LOG_ERROR("Failed to load PBR shaders");
			std::abort();
		}

		FGraphicsPipelineDesc desc{};
		desc.vertexShader = vertexShaderPBR->GetModule();
		desc.fragmentShader = fragmentShaderPBR->GetModule();
		desc.vertexEntryPoint = vertexShaderPBR->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = fragmentShaderPBR->GetEntryPoint().c_str();
		desc.descriptorSetLayouts = { descriptorSetLayout };

		VkPushConstantRange materialRange{};
		materialRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		materialRange.offset = 0;
		materialRange.size = sizeof(FPBRMaterial);
		desc.pushConstantRanges = { materialRange };

		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = mesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);
		desc.vertexAttributes = mesh->GetVertexLayout().attributes;

		desc.colorFormat = ColorFormat;
		desc.depthFormat = DepthFormat;

		pipelinePBR = std::make_unique<VulkanGraphicsPipeline>(device, desc);
	}
}
