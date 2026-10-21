#include "pch.h"
#include "Renderer.h"

#include "Core/Paths.h"
#include "Graphics/VertexFormat.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanSampler.h"
#include "ShaderConventions.h"
#include "Utils/Image.h"

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
		const uint32 width = device.GetSwapchain().GetWidth();
		const uint32 height = device.GetSwapchain().GetHeight();

		if (!CreateSceneColorImage(width, height))
		{
			DL_LOG_ERROR("Failed to create scene-color resources");
			std::abort();
		}

		if (!CreateDepthStencilImage(width, height))
		{
			DL_LOG_ERROR("Failed to create depth-stencil resources");
			std::abort();
		}

		const std::filesystem::path imagePath = Paths::GetAssetPath(L"Image/Test.jpg");
		Image image(imagePath.string());
		testImage = device.CreateImage2D(image);

		// 선형 반복 샘플러 
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		linearRepeatSampler = std::make_unique<VulkanSampler>(device, samplerInfo);

		imageVertexBuffer = device.CreateVertexBuffer(
			imageVertices.data(),
			static_cast<VkDeviceSize>(sizeof(imageVertices[0]) * imageVertices.size()));

		imageIndexBuffer = device.CreateIndexBuffer(
			imageIndices.data(),
			static_cast<VkDeviceSize>(sizeof(imageIndices[0]) * imageIndices.size()));

		triangleVertexBuffer = device.CreateVertexBuffer(
			triangleVertices.data(),
			static_cast<VkDeviceSize>(sizeof(triangleVertices[0]) * triangleVertices.size()));

		triangleIndexBuffer = device.CreateIndexBuffer(
			triangleIndices.data(),
			static_cast<VkDeviceSize>(sizeof(triangleIndices[0]) * triangleIndices.size()));

		VkDescriptorImageInfo textureInfo = {};
		textureInfo.imageView = testImage->GetImageView();
		textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo samplerDescriptorInfo = {};
		samplerDescriptorInfo.sampler = linearRepeatSampler->GetSampler();

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

		linearRepeatSampler.reset();
		testImage.reset();

		DestroyDepthStencilImage();
		DestroySceneColorImage();
	}

	bool Renderer::CreateSceneColorImage(uint32 width, uint32 height)
	{
		DestroySceneColorImage();

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VulkanSwapchain::swapchainFormat;
		imageInfo.extent = { width, height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = imageInfo.format;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		sceneColorImage = std::make_unique<VulkanImage>(device, imageInfo, imageViewInfo);
		return true;
	}

	void Renderer::DestroySceneColorImage()
	{
		sceneColorImage.reset();
	}

	bool Renderer::CreateDepthStencilImage(uint32 width, uint32 height)
	{
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), depthStencilFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			DL_LOG_ERROR("VK_FORMAT_D32_SFLOAT_S8_UINT is not supported as a depth-stencil attachment");
			return false;
		}

		DestroyDepthStencilImage();

		VkImageCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthStencilCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthStencilCreateInfo.format = depthStencilFormat;
		depthStencilCreateInfo.extent.width = width;
		depthStencilCreateInfo.extent.height = height;
		depthStencilCreateInfo.extent.depth = 1;
		depthStencilCreateInfo.mipLevels = 1;
		depthStencilCreateInfo.arrayLayers = 1;
		depthStencilCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthStencilCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthStencilCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthStencilCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		depthStencilCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = depthStencilFormat;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		depthStencilImage = std::make_unique<VulkanImage>(device, depthStencilCreateInfo, imageViewInfo);
		return true;
	}

	void Renderer::DestroyDepthStencilImage()
	{
		depthStencilImage.reset();
	}

	void Renderer::OnResize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		if (!CreateSceneColorImage(width, height))
		{
			DL_LOG_ERROR("Failed to recreate scene-color resources");
			std::abort();
		}

		if (!CreateDepthStencilImage(width, height))
		{
			DL_LOG_ERROR("Failed to recreate depth-stencil resources");
			std::abort();
		}
	}

	bool Renderer::CreateTrianglePipelines()
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

		FGfxPipelineDesc desc{};
		desc.vertexShader = vertModule;
		desc.fragmentShader = fragModule;
		desc.vertexEntryPoint = ShaderConventions::VertexEntry;
		desc.fragmentEntryPoint = ShaderConventions::PixelEntry;


		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = sizeof(VertexFormat::FColoredVertex);
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);

		// dx11 InputLayout 같은 개념 
		desc.vertexAttributes.resize(2);
		desc.vertexAttributes[0].location = 0;
		desc.vertexAttributes[0].binding = 0;
		desc.vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		desc.vertexAttributes[0].offset = offsetof(VertexFormat::FColoredVertex, position);

		desc.vertexAttributes[1].location = 1;
		desc.vertexAttributes[1].binding = 0;
		desc.vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		desc.vertexAttributes[1].offset = offsetof(VertexFormat::FColoredVertex, color);

		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = depthStencilFormat;

		trianglePipeline = std::make_unique<VulkanPipeline>(device, desc);

		vkDestroyShaderModule(device.GetDevice(), vertModule, nullptr);
		vkDestroyShaderModule(device.GetDevice(), fragModule, nullptr);

		return true;
	}

	bool Renderer::CreateImageTestPipelines()
	{
		// 셰이더 컴파일 -> 추후 shaderManager로 관리 
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"ImageTest.hlsl");

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

		FGfxPipelineDesc desc{};
		desc.descriptorSetLayouts =
		{
			device.GetBindlessLayout(),
			device.GetSamplerLayout()
		};

		desc.vertexShader = vertModule;
		desc.fragmentShader = fragModule;
		desc.vertexEntryPoint = ShaderConventions::VertexEntry;
		desc.fragmentEntryPoint = ShaderConventions::PixelEntry;

		VkVertexInputBindingDescription vertexBinding{};
		vertexBinding.binding = 0;
		vertexBinding.stride = sizeof(VertexFormat::FTexturedVertex);
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		desc.vertexBindings.push_back(vertexBinding);

		// dx11 InputLayout 같은 개념 
		desc.vertexAttributes.resize(2);
		desc.vertexAttributes[0].location = 0;
		desc.vertexAttributes[0].binding = 0;
		desc.vertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		desc.vertexAttributes[0].offset = offsetof(VertexFormat::FTexturedVertex, position);

		desc.vertexAttributes[1].location = 1;
		desc.vertexAttributes[1].binding = 0;
		desc.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
		desc.vertexAttributes[1].offset = offsetof(VertexFormat::FTexturedVertex, texcoord);

		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = depthStencilFormat;

		imageTestPipeline = std::make_unique<VulkanPipeline>(device, desc);

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

		if (imageTestPipeline)
		{
			imageTestPipeline.reset();
		}
	}

	void Renderer::Render()
	{
		const VkCommandBuffer commandBuffer = device.GetCurrentCommandBuffer();
		const VkExtent2D extent = {
			sceneColorImage->GetWidth(),
			sceneColorImage->GetHeight()
		};

		std::array<VkImageMemoryBarrier2, 2> layoutBarriers{};

		layoutBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		layoutBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		layoutBarriers[0].srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		layoutBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		layoutBarriers[0].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		layoutBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		layoutBarriers[0].image = sceneColorImage->GetImage();
		layoutBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		layoutBarriers[0].subresourceRange.baseMipLevel = 0;
		layoutBarriers[0].subresourceRange.levelCount = 1;
		layoutBarriers[0].subresourceRange.baseArrayLayer = 0;
		layoutBarriers[0].subresourceRange.layerCount = 1;

		layoutBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		layoutBarriers[1].srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		layoutBarriers[1].srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layoutBarriers[1].dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		layoutBarriers[1].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layoutBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutBarriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		layoutBarriers[1].image = depthStencilImage->GetImage();
		layoutBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		layoutBarriers[1].subresourceRange.baseMipLevel = 0;
		layoutBarriers[1].subresourceRange.levelCount = 1;
		layoutBarriers[1].subresourceRange.baseArrayLayer = 0;
		layoutBarriers[1].subresourceRange.layerCount = 1;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = static_cast<uint32>(layoutBarriers.size());
		dependencyInfo.pImageMemoryBarriers = layoutBarriers.data();
		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

		VkRenderingAttachmentInfo colorAttachmentInfo = {};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = sceneColorImage->GetImageView();
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = { 0.22f, 0.22f, 0.22f, 1.f };

		VkRenderingAttachmentInfo depthAttachmentInfo = {};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = depthStencilImage->GetImageView();
		depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentInfo.clearValue.depthStencil = { 1.f, 0 };

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset.x = 0;
		renderingInfo.renderArea.offset.y = 0;
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		{
			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = static_cast<float>(extent.height);
			viewport.width = static_cast<float>(extent.width);
			// 다렉 좌표계를 따르고 마지막 뷰포트시 음수로 하여서 vulkan 좌표계와 일치
			viewport.height = -static_cast<float>(extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent = extent;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			VkDescriptorSet sets[2] =
			{
				device.GetBindlessSet(),
				device.GetSamplerSet()
			};

			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				imageTestPipeline->GetPipelineLayout(),
				0,
				2,
				sets,
				0,
				nullptr);

			//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline->GetPipeline());
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, imageTestPipeline->GetPipeline());


			const VkBuffer vertexBuffer = imageVertexBuffer->GetBuffer();
			const VkDeviceSize offset = 0;

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
			vkCmdBindIndexBuffer(commandBuffer, imageIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(imageIndices.size()), 1, 0, 0, 0);


		}
		vkCmdEndRendering(commandBuffer);
	}

}
