#include "pch.h"
#include "EnvironmentMapGenerator.h"
#include "Core/Paths.h"
#include "Rendering/PushConstants.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Resources/ShaderResource.h"
#include "Utils/DDSCubemap.h"
#include "Utils/Image.h"

#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanSampler.h"

namespace Dlight
{
	EnvironmentMapGenerator::EnvironmentMapGenerator(VulkanDevice& _device, ShaderCompiler& _shaderCompiler)
		: device(_device)
		, shaderCompiler(_shaderCompiler)
	{
	}

	EnvironmentMapGenerator::~EnvironmentMapGenerator()
	{
		DestroyIrradianceMapResources();
		DestroyCubemapResources();
		irradianceMap.reset();
		cubemap.reset();
	}

	bool EnvironmentMapGenerator::LoadEnvironmentMaps(
		const std::filesystem::path& cubemapPath,
		const std::filesystem::path& irradianceMapPath)
	{
		auto loadedCubemap = DDSCubemap::Load(device, cubemapPath);
		auto loadedIrradianceMap = DDSCubemap::Load(device, irradianceMapPath);
		if (!loadedCubemap || !loadedIrradianceMap)
		{
			return false;
		}

		DestroyIrradianceMapResources();
		DestroyCubemapResources();
		cubemap = std::move(loadedCubemap);
		irradianceMap = std::move(loadedIrradianceMap);
		faceSize = cubemap->GetWidth();
		irradianceMapFaceSize = irradianceMap->GetWidth();
		DL_LOG_INFO("Loaded environment DDS cubemaps");
		return true;
	}

	bool EnvironmentMapGenerator::SaveEnvironmentMaps(
		const std::filesystem::path& cubemapPath,
		const std::filesystem::path& irradianceMapPath) const
	{
		if (!cubemap || !irradianceMap)
		{
			DL_LOG_ERROR("Environment maps must exist before saving DDS files");
			return false;
		}

		return DDSCubemap::Save(device, *cubemap, cubemapPath) &&
			DDSCubemap::Save(device, *irradianceMap, irradianceMapPath);
	}

	bool EnvironmentMapGenerator::CreateCubemap(const Image& srcImage)
	{
		if (!srcImage.IsValid())
		{
			DL_LOG_ERROR("Invalid environment image");

			return false;
		}

		DestroyCubemapResources();

		faceSize = static_cast<uint32>(srcImage.GetHeight() / 2);

		FVulkanImageDesc cubemapDesc = {};

		cubemapDesc.imageType = VK_IMAGE_TYPE_2D;
		cubemapDesc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		cubemapDesc.extent = { faceSize, faceSize, 1 };
		cubemapDesc.format = CubemapFormat;
		cubemapDesc.mipLevels = 1;
		cubemapDesc.arrayLayers = 6;
		cubemapDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		cubemapDesc.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		cubemapDesc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		cubemap = std::make_unique<VulkanImage>(device, cubemapDesc);

		FVulkanImageDesc desc = {};
		desc.imageType = VK_IMAGE_TYPE_2D;
		desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
		desc.extent = {
			static_cast<uint32>(srcImage.GetWidth()),
			static_cast<uint32>(srcImage.GetHeight()),
			1 };

		desc.format = srcImage.GetFormat();
		desc.mipLevels = 1;
		desc.arrayLayers = 1;
		desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		auto srcTexture = std::make_unique<VulkanImage>(device, desc);
		device.UploadImage(*srcTexture, srcImage.GetData(), srcImage.GetSize(), EImageState::FragmentSampledRead);

		CreateCubemapSampler();
		CreateCubemapDescriptorSetLayout();
		CreateCubemapDescriptorPool();
		AllocateCubemapDescriptorSet();
		WriteCubemapDescriptorSet(*srcTexture);

		if (!CreateCubemapGraphicsPipeline(cubemapDescriptorSetLayout))
		{
			DestroyCubemapResources();
			return false;
		}

		RenderCube();

		DestroyCubemapResources();
		return true;
	}

	bool EnvironmentMapGenerator::CreateIrradianceMap()
	{
		if (!cubemap)
		{
			DL_LOG_ERROR("Invalid cubemap");
			return false;
		}

		DestroyIrradianceMapResources();
		irradianceMap.reset();

		FVulkanImageDesc irradianceMapDesc = {};
		irradianceMapDesc.imageType = VK_IMAGE_TYPE_2D;
		irradianceMapDesc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		irradianceMapDesc.extent = { irradianceMapFaceSize, irradianceMapFaceSize, 1 };
		irradianceMapDesc.format = CubemapFormat;
		irradianceMapDesc.mipLevels = 1;
		irradianceMapDesc.arrayLayers = 6;
		irradianceMapDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		irradianceMapDesc.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		irradianceMapDesc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		irradianceMap = std::make_unique<VulkanImage>(device, irradianceMapDesc);

		CreateIrradianceMapSampler();
		CreateIrradianceMapDescriptorSetLayout();
		CreateIrradianceMapDescriptorPool();
		AllocateIrradianceMapDescriptorSet();
		WriteIrradianceMapDescriptorSet(*cubemap);

		if (!CreateIrradianceMapGraphicsPipeline(irradianceMapDescriptorSetLayout))
		{
			DestroyIrradianceMapResources();
			irradianceMap.reset();
			return false;
		}

		RenderIrradianceMap();
		DestroyIrradianceMapResources();
		return true;
	}

	void EnvironmentMapGenerator::CreateIrradianceMapSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		irradianceMapSampler = std::make_unique<VulkanSampler>(device, samplerInfo);
	}

	void EnvironmentMapGenerator::CreateIrradianceMapDescriptorSetLayout()
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

		vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, nullptr, &irradianceMapDescriptorSetLayout);
	}

	void EnvironmentMapGenerator::CreateIrradianceMapDescriptorPool()
	{
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

		vkCreateDescriptorPool(device.GetDevice(), &poolInfo, nullptr, &irradianceMapDescriptorPool);
	}

	void EnvironmentMapGenerator::AllocateIrradianceMapDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = irradianceMapDescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &irradianceMapDescriptorSetLayout;

		vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, &irradianceMapDescriptorSet);
	}

	void EnvironmentMapGenerator::WriteIrradianceMapDescriptorSet(const VulkanImage& sourceCubemap)
	{
		VkDescriptorImageInfo sourceInfo{};
		sourceInfo.imageView = sourceCubemap.GetImageView();
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo samplerInfo{};
		samplerInfo.sampler = irradianceMapSampler->GetSampler();

		std::array<VkWriteDescriptorSet, 2> writes{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = irradianceMapDescriptorSet;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageInfo = &sourceInfo;
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = irradianceMapDescriptorSet;
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pImageInfo = &samplerInfo;

		vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
	}

	bool EnvironmentMapGenerator::CreateIrradianceMapGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout)
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"IrradianceMap.hlsl");

		FShaderRaw vertexShaderRaw{};
		vertexShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::VertexEntry,
			L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		irradianceMapVertexShader = std::make_unique<ShaderResource>(
			"IrradianceMap.VS",
			device,
			std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw{};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::PixelEntry,
			L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		irradianceMapFragmentShader = std::make_unique<ShaderResource>(
			"IrradianceMap.PS",
			device,
			std::move(fragmentShaderRaw));

		if (!irradianceMapVertexShader->Load() || !irradianceMapFragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load irradiance map shaders");
			return false;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(FEnvironmentFacePushConstants);

		FGraphicsPipelineDesc pipelineDesc{};
		pipelineDesc.vertexShader = irradianceMapVertexShader->GetModule();
		pipelineDesc.fragmentShader = irradianceMapFragmentShader->GetModule();
		pipelineDesc.vertexEntryPoint = irradianceMapVertexShader->GetEntryPoint().c_str();
		pipelineDesc.fragmentEntryPoint = irradianceMapFragmentShader->GetEntryPoint().c_str();
		pipelineDesc.descriptorSetLayouts = { descriptorSetLayout };
		pipelineDesc.pushConstantRanges = { pushConstantRange };
		pipelineDesc.cullMode = VK_CULL_MODE_NONE;
		pipelineDesc.colorFormat = CubemapFormat;
		pipelineDesc.depthFormat = VK_FORMAT_UNDEFINED;

		irradianceMapPipeline = std::make_unique<VulkanGraphicsPipeline>(device, pipelineDesc);
		return true;
	}

	void EnvironmentMapGenerator::RenderIrradianceMap()
	{
		VkCommandPool commandPool = { VK_NULL_HANDLE };
		VkCommandBuffer commandBuffer = { VK_NULL_HANDLE };

		VkCommandPoolCreateInfo commandPoolInfo = {};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		commandPoolInfo.queueFamilyIndex = device.GetGraphicsAndComputeQueueFamilyIndex();
		vkCreateCommandPool(device.GetDevice(), &commandPoolInfo, nullptr, &commandPool);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(device.GetDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageMemoryBarrier2 irradianceBeginBarrier = {};
		irradianceBeginBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		irradianceBeginBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		irradianceBeginBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		irradianceBeginBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		irradianceBeginBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		irradianceBeginBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		irradianceBeginBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		irradianceBeginBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		irradianceBeginBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		irradianceBeginBarrier.image = irradianceMap->GetImage();
		irradianceBeginBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		irradianceBeginBarrier.subresourceRange.baseMipLevel = 0;
		irradianceBeginBarrier.subresourceRange.levelCount = 1;
		irradianceBeginBarrier.subresourceRange.baseArrayLayer = 0;
		irradianceBeginBarrier.subresourceRange.layerCount = 6;

		VkDependencyInfo irradianceBeginDependency = {};
		irradianceBeginDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		irradianceBeginDependency.imageMemoryBarrierCount = 1;
		irradianceBeginDependency.pImageMemoryBarriers = &irradianceBeginBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &irradianceBeginDependency);

		VkViewport viewport{};
		viewport.y = static_cast<float>(irradianceMapFaceSize);
		viewport.width = static_cast<float>(irradianceMapFaceSize);
		viewport.height = -static_cast<float>(irradianceMapFaceSize);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { irradianceMapFaceSize, irradianceMapFaceSize };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, irradianceMapPipeline->GetPipeline());
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			irradianceMapPipeline->GetPipelineLayout(),
			0, 1, &irradianceMapDescriptorSet, 0, nullptr);

		std::array<VkImageView, 6> faceViews = {};

		for (uint32 i = 0; i < static_cast<uint32>(faceViews.size()); ++i)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = irradianceMap->GetImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = CubemapFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = i;
			viewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &faceViews[i]);
		}

		for (uint32 faceIndex = 0; faceIndex < static_cast<uint32>(faceViews.size()); ++faceIndex)
		{
			VkRenderingAttachmentInfo colorAttachment{};
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.imageView = faceViews[faceIndex];
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

			VkRenderingInfo renderingInfo{};
			renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			renderingInfo.renderArea.extent = { irradianceMapFaceSize, irradianceMapFaceSize };
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &colorAttachment;

			vkCmdBeginRendering(commandBuffer, &renderingInfo);

			FEnvironmentFacePushConstants pushConstants{};
			pushConstants.faceIndex = faceIndex;
			vkCmdPushConstants(
				commandBuffer,
				irradianceMapPipeline->GetPipelineLayout(),
				VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(pushConstants),
				&pushConstants);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			vkCmdEndRendering(commandBuffer);
		}

		VkImageMemoryBarrier2 irradianceEndBarrier = {};
		irradianceEndBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		irradianceEndBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		irradianceEndBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		irradianceEndBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		irradianceEndBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		irradianceEndBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		irradianceEndBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		irradianceEndBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		irradianceEndBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		irradianceEndBarrier.image = irradianceMap->GetImage();
		irradianceEndBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		irradianceEndBarrier.subresourceRange.baseMipLevel = 0;
		irradianceEndBarrier.subresourceRange.levelCount = 1;
		irradianceEndBarrier.subresourceRange.baseArrayLayer = 0;
		irradianceEndBarrier.subresourceRange.layerCount = 6;

		VkDependencyInfo irradianceEndDependency = {};
		irradianceEndDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		irradianceEndDependency.imageMemoryBarrierCount = 1;
		irradianceEndDependency.pImageMemoryBarriers = &irradianceEndBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &irradianceEndDependency);

		vkEndCommandBuffer(commandBuffer);

		VkCommandBufferSubmitInfo commandSubmitInfo{};
		commandSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandSubmitInfo.commandBuffer = commandBuffer;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandSubmitInfo;

		vkQueueSubmit2(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkDeviceWaitIdle(device.GetDevice());
		vkDestroyCommandPool(device.GetDevice(), commandPool, nullptr);

		for (VkImageView faceView : faceViews)
		{
			if (faceView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(device.GetDevice(), faceView, nullptr);
			}
		}
	}

	void EnvironmentMapGenerator::DestroyIrradianceMapResources()
	{
		irradianceMapPipeline.reset();
		irradianceMapFragmentShader.reset();
		irradianceMapVertexShader.reset();
		irradianceMapSampler.reset();

		if (irradianceMapDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device.GetDevice(), irradianceMapDescriptorPool, nullptr);
			irradianceMapDescriptorPool = VK_NULL_HANDLE;
		}

		irradianceMapDescriptorSet = VK_NULL_HANDLE;

		if (irradianceMapDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device.GetDevice(), irradianceMapDescriptorSetLayout, nullptr);
			irradianceMapDescriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	void EnvironmentMapGenerator::CreateCubemapSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		cubemapSampler = std::make_unique<VulkanSampler>(device, samplerInfo);
	}

	void EnvironmentMapGenerator::CreateCubemapDescriptorSetLayout()
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

		vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, nullptr, &cubemapDescriptorSetLayout);
	}

	void EnvironmentMapGenerator::CreateCubemapDescriptorPool()
	{
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

		vkCreateDescriptorPool(
			device.GetDevice(),
			&poolInfo,
			nullptr,
			&cubemapDescriptorPool);
	}

	void EnvironmentMapGenerator::AllocateCubemapDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = cubemapDescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &cubemapDescriptorSetLayout;

		vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, &cubemapDescriptorSet);
	}

	void EnvironmentMapGenerator::WriteCubemapDescriptorSet(const VulkanImage& sourceTexture)
	{
		VkDescriptorImageInfo sourceInfo{};
		sourceInfo.imageView = sourceTexture.GetImageView();
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorImageInfo samplerInfo{};
		samplerInfo.sampler = cubemapSampler->GetSampler();

		std::array<VkWriteDescriptorSet, 2> writes{};
		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = cubemapDescriptorSet;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writes[0].pImageInfo = &sourceInfo;
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = cubemapDescriptorSet;
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writes[1].pImageInfo = &samplerInfo;

		vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
	}

	bool EnvironmentMapGenerator::CreateCubemapGraphicsPipeline(VkDescriptorSetLayout descriptorSetLayout)
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"EquirectangularToCubemap.hlsl");

		FShaderRaw vertexShaderRaw{};
		vertexShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::VertexEntry,
			L"vs_6_0");
		vertexShaderRaw.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderRaw.entryPoint = ShaderConventions::VertexEntry;
		cubemapVertexShader = std::make_unique<ShaderResource>(
			"EquirectangularToCubemap.VS",
			device,
			std::move(vertexShaderRaw));

		FShaderRaw fragmentShaderRaw{};
		fragmentShaderRaw.spirv = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::PixelEntry,
			L"ps_6_0");
		fragmentShaderRaw.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderRaw.entryPoint = ShaderConventions::PixelEntry;
		cubemapFragmentShader = std::make_unique<ShaderResource>(
			"EquirectangularToCubemap.PS",
			device,
			std::move(fragmentShaderRaw));

		if (!cubemapVertexShader->Load() || !cubemapFragmentShader->Load())
		{
			DL_LOG_ERROR("Failed to load cubemap shaders");
			return false;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(FEnvironmentFacePushConstants);

		FGraphicsPipelineDesc pipelineDesc{};
		pipelineDesc.vertexShader = cubemapVertexShader->GetModule();
		pipelineDesc.fragmentShader = cubemapFragmentShader->GetModule();
		pipelineDesc.vertexEntryPoint = cubemapVertexShader->GetEntryPoint().c_str();
		pipelineDesc.fragmentEntryPoint = cubemapFragmentShader->GetEntryPoint().c_str();
		pipelineDesc.descriptorSetLayouts = { descriptorSetLayout };
		pipelineDesc.pushConstantRanges = { pushConstantRange };
		pipelineDesc.cullMode = VK_CULL_MODE_NONE;
		pipelineDesc.colorFormat = CubemapFormat;
		pipelineDesc.depthFormat = VK_FORMAT_UNDEFINED;

		cubemapPipeline = std::make_unique<VulkanGraphicsPipeline>(device, pipelineDesc);
		return true;
	}

	void EnvironmentMapGenerator::RenderCube()
	{
		VkCommandPool commandPool = VK_NULL_HANDLE;

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		poolInfo.queueFamilyIndex = device.GetGraphicsAndComputeQueueFamilyIndex();

		vkCreateCommandPool(device.GetDevice(), &poolInfo, nullptr, &commandPool);

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(device.GetDevice(), &allocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageMemoryBarrier2 beginBarrier{};
		beginBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		beginBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		beginBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		beginBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		beginBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		beginBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		beginBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		beginBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beginBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		beginBarrier.image = cubemap->GetImage();
		beginBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		beginBarrier.subresourceRange.baseMipLevel = 0;
		beginBarrier.subresourceRange.levelCount = 1;
		beginBarrier.subresourceRange.baseArrayLayer = 0;
		beginBarrier.subresourceRange.layerCount = 6;

		VkDependencyInfo beginDependency{};
		beginDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		beginDependency.imageMemoryBarrierCount = 1;
		beginDependency.pImageMemoryBarriers = &beginBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &beginDependency);

		VkViewport viewport{};
		viewport.y = static_cast<float>(faceSize);
		viewport.width = static_cast<float>(faceSize);
		viewport.height = -static_cast<float>(faceSize);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { faceSize, faceSize };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			cubemapPipeline->GetPipeline());

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			cubemapPipeline->GetPipelineLayout(),
			0, 1, &cubemapDescriptorSet, 0, nullptr);

		std::array<VkImageView, 6> faceViews = {};

		for (uint32 i = 0; i < static_cast<uint32>(faceViews.size()); ++i)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = cubemap->GetImage();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = CubemapFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = i;
			viewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &faceViews[i]);
		}

		for (uint32 faceIndex = 0; faceIndex < static_cast<uint32>(faceViews.size()); ++faceIndex)
		{
			VkRenderingAttachmentInfo colorAttachment{};
			colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorAttachment.imageView = faceViews[faceIndex];
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

			VkRenderingInfo renderingInfo{};
			renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			renderingInfo.renderArea.extent = { faceSize, faceSize };
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &colorAttachment;

			vkCmdBeginRendering(commandBuffer, &renderingInfo);

			FEnvironmentFacePushConstants pushConstants{};
			pushConstants.faceIndex = faceIndex;
			vkCmdPushConstants(
				commandBuffer,
				cubemapPipeline->GetPipelineLayout(),
				VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(pushConstants),
				&pushConstants);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
			vkCmdEndRendering(commandBuffer);
		}

		VkImageMemoryBarrier2 endBarrier{};
		endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		endBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		endBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		endBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		endBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		endBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		endBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		endBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		endBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		endBarrier.image = cubemap->GetImage();
		endBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		endBarrier.subresourceRange.baseMipLevel = 0;
		endBarrier.subresourceRange.levelCount = 1;
		endBarrier.subresourceRange.baseArrayLayer = 0;
		endBarrier.subresourceRange.layerCount = 6;

		VkDependencyInfo endDependency{};
		endDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		endDependency.imageMemoryBarrierCount = 1;
		endDependency.pImageMemoryBarriers = &endBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &endDependency);

		vkEndCommandBuffer(commandBuffer);

		VkCommandBufferSubmitInfo commandSubmitInfo{};
		commandSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandSubmitInfo.commandBuffer = commandBuffer;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandSubmitInfo;

		vkQueueSubmit2(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkDeviceWaitIdle(device.GetDevice());
		vkDestroyCommandPool(device.GetDevice(), commandPool, nullptr);

		for (VkImageView faceView : faceViews)
		{
			if (faceView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(device.GetDevice(), faceView, nullptr);
			}
		}
	}

	void EnvironmentMapGenerator::DestroyCubemapResources()
	{
		cubemapPipeline.reset();
		cubemapFragmentShader.reset();
		cubemapVertexShader.reset();
		cubemapSampler.reset();

		if (cubemapDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device.GetDevice(), cubemapDescriptorPool, nullptr);
			cubemapDescriptorPool = VK_NULL_HANDLE;
		}

		cubemapDescriptorSet = VK_NULL_HANDLE;

		if (cubemapDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device.GetDevice(), cubemapDescriptorSetLayout, nullptr);
			cubemapDescriptorSetLayout = VK_NULL_HANDLE;
		}
	}
}
