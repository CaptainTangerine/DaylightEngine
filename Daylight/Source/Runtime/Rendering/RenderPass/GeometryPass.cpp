#include "pch.h"
#include "GeometryPass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Resources/MeshResource.h"
#include "Resources/ResourceManager.h"
#include "Resources/ShaderResource.h"

namespace Dlight
{
	GeometryPass::GeometryPass(VulkanDevice& _device, ResourceManager& _resourceManager, ShaderCompiler& _shaderCompiler)
		: device(_device), resourceManager(_resourceManager), shaderCompiler(_shaderCompiler)
	{
		CreateMesh();
		CreateViewResources();
		CreatePipeline();
	}

	GeometryPass::~GeometryPass()
	{
		DestroyViewResources();
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
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

		const uint32 frameSlot = device.GetCurrentFrameResource().frameSlot;
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetPipelineLayout(),
			0, 1, &viewDescriptorSets[frameSlot], 0, nullptr
		);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

		const VkBuffer vertexBuffer = mesh->GetVertexBuffer()->GetBuffer();
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer()->GetBuffer(), 0, mesh->GetIndexType());
		vkCmdDrawIndexed(commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);

		vkCmdEndRendering(commandBuffer);
	}

	void GeometryPass::CreateViewResources()
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &binding;

		if (VK_SUCCESS != vkCreateDescriptorSetLayout(
			device.GetDevice(),
			&layoutInfo,
			nullptr,
			&viewDescriptorSetLayout))
		{
			DL_LOG_ERROR("Failed to create geometry view descriptor set layout");
			std::abort();
		}

		constexpr uint32 frameCount = VulkanDevice::MaxFramesInFlight;

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = frameCount;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = frameCount;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;

		if (VK_SUCCESS != vkCreateDescriptorPool(
			device.GetDevice(),
			&poolInfo,
			nullptr,
			&viewDescriptorPool))
		{
			DL_LOG_ERROR("Failed to create geometry view descriptor pool");
			std::abort();
		}

		std::array<VkDescriptorSetLayout, frameCount> layouts{};
		layouts.fill(viewDescriptorSetLayout);
		viewDescriptorSets.resize(frameCount);

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = viewDescriptorPool;
		allocateInfo.descriptorSetCount = frameCount;
		allocateInfo.pSetLayouts = layouts.data();

		if (VK_SUCCESS != vkAllocateDescriptorSets(
			device.GetDevice(),
			&allocateInfo,
			viewDescriptorSets.data()))
		{
			DL_LOG_ERROR("Failed to allocate geometry view descriptor sets");
			std::abort();
		}

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

			auto uniformBuffer = std::make_unique<VulkanBuffer>(device, bufferDesc);
			if (uniformBuffer->GetMappedData() == nullptr)
			{
				DL_LOG_ERROR("Geometry view uniform buffer is not mapped");
				std::abort();
			}

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffer->GetBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(FPerViewUniform);

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = viewDescriptorSets[frameSlot];
			write.dstBinding = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(device.GetDevice(), 1, &write, 0, nullptr);
			viewUniformBuffers.push_back(std::move(uniformBuffer));
		}
	}

	void GeometryPass::DestroyViewResources()
	{
		if (viewDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device.GetDevice(), viewDescriptorPool, nullptr);
			viewDescriptorPool = VK_NULL_HANDLE;
		}
		viewDescriptorSets.clear();

		if (viewDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device.GetDevice(), viewDescriptorSetLayout, nullptr);
			viewDescriptorSetLayout = VK_NULL_HANDLE;
		}

		viewUniformBuffers.clear();
	}

	void GeometryPass::CreateMesh()
	{
		FMeshRaw meshData = {};
		meshData.vertexData.resize(sizeof(vertices[0]) * vertices.size());
		std::memcpy(meshData.vertexData.data(), vertices.data(), meshData.vertexData.size());
		meshData.indices = indices;
		meshData.layout.stride = sizeof(VertexFormat::FColoredVertex);
		meshData.layout.attributes.resize(2);
		meshData.layout.attributes[0].location = 0;
		meshData.layout.attributes[0].binding = 0;
		meshData.layout.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		meshData.layout.attributes[0].offset = offsetof(VertexFormat::FColoredVertex, position);
		meshData.layout.attributes[1].location = 1;
		meshData.layout.attributes[1].binding = 0;
		meshData.layout.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		meshData.layout.attributes[1].offset = offsetof(VertexFormat::FColoredVertex, color);

		mesh = std::make_unique<MeshResource>("Cube", device, std::move(meshData));
		if (!mesh->Load())
		{
			DL_LOG_ERROR("Failed to load cube mesh");
			std::abort();
		}
	}

	void GeometryPass::UpdateViewData(const FViewData& viewData)
	{
		const FPerViewUniform uniformData =
		{
			viewData.viewMatrix,
			viewData.projectionMatrix
		};

		const uint32 frameSlot = device.GetCurrentFrameResource().frameSlot;

		VulkanBuffer& viewBuffer = *viewUniformBuffers[frameSlot];
		std::memcpy(viewBuffer.GetMappedData(), &uniformData, sizeof(uniformData));
		viewBuffer.FlushAllocation(0, sizeof(uniformData));
	}

	void GeometryPass::CreatePipeline()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Geometry.hlsl");

		FShaderRaw vertexShaderRaw = {};
		vertexShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::VertexEntry, L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShader = std::make_unique<ShaderResource>("Geometry.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw = {};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::PixelEntry, L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShader = std::make_unique<ShaderResource>("Geometry.PS", device, std::move(fragmentShaderRaw));

		if (!vertexShader->Load() || !fragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load geometry shaders");
			std::abort();
		}

		FGraphicsPipelineDesc desc{};
		desc.vertexShader = vertexShader->GetModule();
		desc.fragmentShader = fragmentShader->GetModule();
		desc.vertexEntryPoint = vertexShader->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = fragmentShader->GetEntryPoint().c_str();
		desc.descriptorSetLayouts = { viewDescriptorSetLayout };

		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = mesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);
		desc.vertexAttributes = mesh->GetVertexLayout().attributes;

		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = DepthFormat;

		pipeline = std::make_unique<VulkanGraphicsPipeline>(device, desc);
	}
}
