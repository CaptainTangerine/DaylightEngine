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
		CreatePipeline();
	}

	GeometryPass::~GeometryPass()
	{
	}

	void GeometryPass::Execute(VkCommandBuffer commandBuffer, VkImageView renderTargetView, VkExtent2D extent)
	{
		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = renderTargetView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = { 0.22f, 0.22f, 0.22f, 1.f };

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

		const VkBuffer vertexBuffer = mesh->GetVertexBuffer()->GetBuffer();
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, mesh->GetIndexBuffer()->GetBuffer(), 0, mesh->GetIndexType());
		vkCmdDrawIndexed(commandBuffer, mesh->GetIndexCount(), 1, 0, 0, 0);

		vkCmdEndRendering(commandBuffer);
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
		meshData.layout.attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		meshData.layout.attributes[0].offset = offsetof(VertexFormat::FColoredVertex, position);
		meshData.layout.attributes[1].location = 1;
		meshData.layout.attributes[1].binding = 0;
		meshData.layout.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		meshData.layout.attributes[1].offset = offsetof(VertexFormat::FColoredVertex, color);

		mesh = std::make_unique<MeshResource>("Triangle", device, std::move(meshData));
		if (!mesh->Load())
		{
			DL_LOG_ERROR("Failed to load triangle mesh");
			std::abort();
		}
	}

	void GeometryPass::CreatePipeline()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Triangle.hlsl");

		FShaderRaw vertexShaderRaw = {};
		vertexShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::VertexEntry, L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		vertexShader = std::make_unique<ShaderResource>("Triangle.VS", device, std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw = {};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::PixelEntry, L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		fragmentShader = std::make_unique<ShaderResource>("Triangle.PS", device, std::move(fragmentShaderRaw));

		if (!vertexShader->Load() || !fragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load triangle shaders");
			std::abort();
		}

		FGraphicsPipelineDesc desc{};
		desc.vertexShader = vertexShader->GetModule();
		desc.fragmentShader = fragmentShader->GetModule();
		desc.vertexEntryPoint = vertexShader->GetEntryPoint().c_str();
		desc.fragmentEntryPoint = fragmentShader->GetEntryPoint().c_str();

		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = mesh->GetVertexLayout().stride;
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);
		desc.vertexAttributes = mesh->GetVertexLayout().attributes;

		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = VK_FORMAT_UNDEFINED;

		pipeline = std::make_unique<VulkanGraphicsPipeline>(device, desc);
	}
}
