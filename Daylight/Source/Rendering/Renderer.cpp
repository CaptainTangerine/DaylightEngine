#include "pch.h"
#include "Renderer.h"
#include "GPUProfiler.h"
#include "PushConstants.h"

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
		const uint32 width = device.GetSwapchain().GetWidth();
		const uint32 height = device.GetSwapchain().GetHeight();

		if (!CreateSceneColorImages(width, height))
		{
			DL_LOG_ERROR("Failed to create scene-color resources");
			std::abort();
		}

		if (!CreateDepthStencilImages(width, height))
		{
			DL_LOG_ERROR("Failed to create depth-stencil resources");
			std::abort();
		}

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

		// Blur
		CreateBlurPipeline();
		CreateComputeDescriptorSets();
		UpdateComputeDescriptorSets();
	}

	void Renderer::Shutdown()
	{
		DestroyPipelines();
		DestroyComputeDescriptors();

		triangleVertexShader.reset();
		triangleFragmentShader.reset();
		imageVertexShader.reset();
		imageFragmentShader.reset();

		testTexture.reset();

		DestroyDepthStencilImages();
		DestroySceneColorImages();
	}

	bool Renderer::CreateSceneColorImages(uint32 width, uint32 height)
	{
		DestroySceneColorImages();

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VulkanSwapchain::swapchainFormat;
		imageInfo.extent = { width, height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageCreateInfo computeOutputImageInfo = imageInfo;
		computeOutputImageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		computeOutputImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		VkImageCreateInfo horizontalBlurImageInfo = computeOutputImageInfo;
		horizontalBlurImageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = imageInfo.format;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		VkImageViewCreateInfo computeOutputImageViewInfo = imageViewInfo;
		computeOutputImageViewInfo.format = computeOutputImageInfo.format;

		sceneColorImages.reserve(VulkanDevice::MaxFramesInFlight);
		horizontalBlurImages.reserve(VulkanDevice::MaxFramesInFlight);
		computeOutputImages.reserve(VulkanDevice::MaxFramesInFlight);
		for (uint32 frameSlot = 0; frameSlot < VulkanDevice::MaxFramesInFlight; ++frameSlot)
		{
			sceneColorImages.push_back(std::make_unique<VulkanImage>(device, imageInfo, imageViewInfo));
			horizontalBlurImages.push_back(std::make_unique<VulkanImage>(device, horizontalBlurImageInfo, computeOutputImageViewInfo));
			computeOutputImages.push_back(std::make_unique<VulkanImage>(device, computeOutputImageInfo, computeOutputImageViewInfo));
		}

		return true;
	}

	void Renderer::DestroySceneColorImages()
	{
		sceneColorImages.clear();
		horizontalBlurImages.clear();
		computeOutputImages.clear();
	}

	bool Renderer::CreateDepthStencilImages(uint32 width, uint32 height)
	{
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), depthStencilFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			DL_LOG_ERROR("VK_FORMAT_D32_SFLOAT_S8_UINT is not supported as a depth-stencil attachment");
			return false;
		}

		DestroyDepthStencilImages();

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

		depthStencilImages.reserve(VulkanDevice::MaxFramesInFlight);
		for (uint32 frameSlot = 0; frameSlot < VulkanDevice::MaxFramesInFlight; ++frameSlot)
		{
			depthStencilImages.push_back(
				std::make_unique<VulkanImage>(device, depthStencilCreateInfo, imageViewInfo));
		}

		return true;
	}

	void Renderer::DestroyDepthStencilImages()
	{
		depthStencilImages.clear();
	}

	void Renderer::OnResize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		if (!CreateSceneColorImages(width, height))
		{
			DL_LOG_ERROR("Failed to recreate scene-color resources");
			std::abort();
		}

		if (!CreateDepthStencilImages(width, height))
		{
			DL_LOG_ERROR("Failed to recreate depth-stencil resources");
			std::abort();
		}

		UpdateComputeDescriptorSets();
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

		FGraphicsPipelineDesc desc{};
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
		desc.depthFormat = depthStencilFormat;

		trianglePipeline = std::make_unique<VulkanGraphicsPipeline>(device, desc);

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

		FGraphicsPipelineDesc desc{};
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
		desc.depthFormat = depthStencilFormat;

		imageTestPipeline = std::make_unique<VulkanGraphicsPipeline>(device, desc);

		return true;
	}

	bool Renderer::CreateBlurPipeline()
	{
		const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Blur.hlsl");
		auto compCode = shaderCompiler.Compile(
			shaderPath.c_str(),
			ShaderConventions::ComputeEntry,
			L"cs_6_0");

		VkShaderModule compModule = VK_NULL_HANDLE;
		
		VkShaderModuleCreateInfo compModuleInfo{};
		compModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		compModuleInfo.codeSize = compCode.size() * sizeof(uint32);
		compModuleInfo.pCode = compCode.data();

		if (VK_SUCCESS != vkCreateShaderModule(device.GetDevice(), &compModuleInfo, nullptr, &compModule))
		{
			DL_LOG_ERROR("Failed to create vertex shader module");
			std::abort();
		}

		FComputePipelineDesc pipelineDesc = {};
		pipelineDesc.computeShader = compModule;
		pipelineDesc.computeEntryPoint = ShaderConventions::ComputeEntry;
		
		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
		bindings[0].binding = 0;
		bindings[0].descriptorCount = 1;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorCount = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		if (VK_SUCCESS != vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, nullptr, &computeDescriptorSetLayout))
		{
			DL_LOG_ERROR("Failed to create compute descriptor set layout");
			std::abort();
		}

		pipelineDesc.descriptorSetLayouts = {
			computeDescriptorSetLayout
		};

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(FBlurPushConstants);
		pipelineDesc.pushConstantRanges = { pushConstantRange };

		blurPipeline = std::make_unique<VulkanComputePipeline>(device, pipelineDesc);
		vkDestroyShaderModule(device.GetDevice(), compModule, nullptr);

		return true;
	}

	bool Renderer::CreateComputeDescriptorSets()
	{
		horizontalBlurDescriptorSets.resize(VulkanDevice::MaxFramesInFlight);
		verticalBlurDescriptorSets.resize(VulkanDevice::MaxFramesInFlight);

		// DescriptorPool 
		constexpr uint32 frameCount = VulkanDevice::MaxFramesInFlight;
		constexpr uint32 descriptorSetCount = frameCount * 2;

		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].descriptorCount = descriptorSetCount;
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[1].descriptorCount = descriptorSetCount;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = descriptorSetCount;
		poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		if (VK_SUCCESS != vkCreateDescriptorPool(device.GetDevice(), &poolInfo, nullptr, &computeDescriptorPool))
		{
			DL_LOG_ERROR("Fail to Create compute descriptorSet Pool");
			std::abort();
		}

		std::array<VkDescriptorSetLayout, frameCount> layouts;
		layouts.fill(computeDescriptorSetLayout);

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = computeDescriptorPool;
		allocateInfo.descriptorSetCount = frameCount;
		allocateInfo.pSetLayouts = layouts.data();

		if (VK_SUCCESS != vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, horizontalBlurDescriptorSets.data()))
		{
			DL_LOG_ERROR("Failed to allocate horizontal blur descriptor sets");
			std::abort();
		}

		if (VK_SUCCESS != vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, verticalBlurDescriptorSets.data()))
		{
			DL_LOG_ERROR("Failed to allocate vertical blur descriptor sets");
			std::abort();
		}

		
		return true;
	}

	void Renderer::UpdateComputeDescriptorSets()
	{
		const uint32 frameCount = VulkanDevice::MaxFramesInFlight;

		for (uint32 frameSlot = 0; frameSlot < frameCount; ++frameSlot)
		{
			VkDescriptorImageInfo horizontalInputInfo{};
			horizontalInputInfo.imageView = sceneColorImages[frameSlot]->GetImageView();
			horizontalInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo horizontalOutputInfo{};
			horizontalOutputInfo.imageView = horizontalBlurImages[frameSlot]->GetImageView();
			horizontalOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			VkDescriptorImageInfo verticalInputInfo{};
			verticalInputInfo.imageView = horizontalBlurImages[frameSlot]->GetImageView();
			verticalInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo verticalOutputInfo{};
			verticalOutputInfo.imageView = computeOutputImages[frameSlot]->GetImageView();
			verticalOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			std::array<VkWriteDescriptorSet, 4> writes = {};
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = horizontalBlurDescriptorSets[frameSlot];
			writes[0].dstBinding = 0;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[0].pImageInfo = &horizontalInputInfo;

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = horizontalBlurDescriptorSets[frameSlot];
			writes[1].dstBinding = 1;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[1].pImageInfo = &horizontalOutputInfo;

			writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[2].dstSet = verticalBlurDescriptorSets[frameSlot];
			writes[2].dstBinding = 0;
			writes[2].descriptorCount = 1;
			writes[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writes[2].pImageInfo = &verticalInputInfo;

			writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[3].dstSet = verticalBlurDescriptorSets[frameSlot];
			writes[3].dstBinding = 1;
			writes[3].descriptorCount = 1;
			writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[3].pImageInfo = &verticalOutputInfo;

			vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
		}
	}

	void Renderer::DestroyComputeDescriptors()
	{
		if (computeDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device.GetDevice(), computeDescriptorPool, nullptr);
			computeDescriptorPool = VK_NULL_HANDLE;
		}

		horizontalBlurDescriptorSets.clear();
		verticalBlurDescriptorSets.clear();

		if (computeDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device.GetDevice(), computeDescriptorSetLayout, nullptr);
			computeDescriptorSetLayout = VK_NULL_HANDLE;
		}
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

		if (blurPipeline)
		{
			blurPipeline.reset();
		}
	}

	void Renderer::TransitionScenePassAttachments(VkCommandBuffer commandBuffer)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();
		VulkanImage& depthStencilImage = *depthStencilImages[frameResource.frameSlot];

		std::array<VkImageMemoryBarrier2, 2> layoutBarriers{};

		layoutBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		layoutBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		layoutBarriers[0].srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		layoutBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		layoutBarriers[0].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		layoutBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		layoutBarriers[0].image = sceneColorImages[frameResource.frameSlot]->GetImage();
		layoutBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		layoutBarriers[0].subresourceRange.baseMipLevel = 0;
		layoutBarriers[0].subresourceRange.levelCount = 1;
		layoutBarriers[0].subresourceRange.baseArrayLayer = 0;
		layoutBarriers[0].subresourceRange.layerCount = 1;

		// DepthStencil
		layoutBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		layoutBarriers[1].srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		layoutBarriers[1].srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layoutBarriers[1].dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		layoutBarriers[1].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layoutBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutBarriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		layoutBarriers[1].image = depthStencilImage.GetImage();
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
	}

	void Renderer::ScenePass(VkCommandBuffer commandBuffer)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();
		VulkanImage& depthStencilImage = *depthStencilImages[frameResource.frameSlot];
		const VkExtent2D extent = {
			sceneColorImages[frameResource.frameSlot]->GetWidth(),
			sceneColorImages[frameResource.frameSlot]->GetHeight()
		};

		TransitionScenePassAttachments(commandBuffer);

		VkRenderingAttachmentInfo colorAttachmentInfo = {};
		colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachmentInfo.imageView = sceneColorImages[frameResource.frameSlot]->GetImageView();
		colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentInfo.clearValue.color = { 0.22f, 0.22f, 0.22f, 1.f };

		VkRenderingAttachmentInfo depthAttachmentInfo = {};
		depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachmentInfo.imageView = depthStencilImage.GetImageView();
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

		const VkBuffer vertexBuffer = imageMesh->GetVertexBuffer()->GetBuffer();
		const VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, imageMesh->GetIndexBuffer()->GetBuffer(), 0, imageMesh->GetIndexType());
		vkCmdDrawIndexed(commandBuffer, imageMesh->GetIndexCount(), 1, 0, 0, 0);

		vkCmdEndRendering(commandBuffer);
	}

	void Renderer::TransitionHorizontalBlurPassImages(VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier2 computeInputBarrier = {};
		FFrameResource& resource = device.GetCurrentFrameResource();

		computeInputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		computeInputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;;
		computeInputBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		computeInputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		computeInputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		computeInputBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		computeInputBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		computeInputBarrier.image = sceneColorImages[resource.frameSlot]->GetImage();
		computeInputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		computeInputBarrier.subresourceRange.baseMipLevel = 0;
		computeInputBarrier.subresourceRange.levelCount = 1;
		computeInputBarrier.subresourceRange.baseArrayLayer = 0;
		computeInputBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo computeInputDepInfo = {};
		computeInputDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		computeInputDepInfo.imageMemoryBarrierCount = 1;
		computeInputDepInfo.pImageMemoryBarriers = &computeInputBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &computeInputDepInfo);

		VkImageMemoryBarrier2 computeOutputBarrier = {};

		computeOutputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		computeOutputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		computeOutputBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		computeOutputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		computeOutputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		computeOutputBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		computeOutputBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		computeOutputBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		computeOutputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		computeOutputBarrier.image = horizontalBlurImages[resource.frameSlot]->GetImage();
		computeOutputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		computeOutputBarrier.subresourceRange.baseMipLevel = 0;
		computeOutputBarrier.subresourceRange.levelCount = 1;
		computeOutputBarrier.subresourceRange.baseArrayLayer = 0;
		computeOutputBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo computeOutputDepInfo = {};
		computeOutputDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		computeOutputDepInfo.imageMemoryBarrierCount = 1;
		computeOutputDepInfo.pImageMemoryBarriers = &computeOutputBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &computeOutputDepInfo);

	}

	void Renderer::TransitionVerticalBlurPassImages(VkCommandBuffer commandBuffer)
	{
		VkImageMemoryBarrier2 computeInputBarrier = {};
		FFrameResource& resource = device.GetCurrentFrameResource();

		computeInputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		computeInputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;;
		computeInputBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		computeInputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		computeInputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
		computeInputBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		computeInputBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		computeInputBarrier.image = horizontalBlurImages[resource.frameSlot]->GetImage();
		computeInputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		computeInputBarrier.subresourceRange.baseMipLevel = 0;
		computeInputBarrier.subresourceRange.levelCount = 1;
		computeInputBarrier.subresourceRange.baseArrayLayer = 0;
		computeInputBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo sceneDepInfo = {};
		sceneDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		sceneDepInfo.imageMemoryBarrierCount = 1;
		sceneDepInfo.pImageMemoryBarriers = &computeInputBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &sceneDepInfo);

		VkImageMemoryBarrier2 computeOutputBarrier = {};

		computeOutputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		computeOutputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		computeOutputBarrier.srcAccessMask = VK_ACCESS_2_NONE;
		computeOutputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		computeOutputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		computeOutputBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		computeOutputBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		computeOutputBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		computeOutputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		computeOutputBarrier.image = computeOutputImages[resource.frameSlot]->GetImage();
		computeOutputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		computeOutputBarrier.subresourceRange.baseMipLevel = 0;
		computeOutputBarrier.subresourceRange.levelCount = 1;
		computeOutputBarrier.subresourceRange.baseArrayLayer = 0;
		computeOutputBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo computeDepInfo = {};
		computeDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		computeDepInfo.imageMemoryBarrierCount = 1;
		computeDepInfo.pImageMemoryBarriers = &computeOutputBarrier;
		vkCmdPipelineBarrier2(commandBuffer, &computeDepInfo);
	}

	void Renderer::BlurPass(VkCommandBuffer commandBuffer)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();

		// SceneColor 읽기와 가로 블러 중간 이미지 쓰기를 준비한다.
		TransitionHorizontalBlurPassImages(commandBuffer);
		FBlurPushConstants constants{ { 1, 0 } };
		
		vkCmdPushConstants(commandBuffer, blurPipeline->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blurPipeline->GetPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blurPipeline->GetPipelineLayout(), 0, 1, &horizontalBlurDescriptorSets[frameResource.frameSlot], 0, nullptr);

		constexpr uint32 threadGroupSizeX = 8;
		constexpr uint32 threadGroupSizeY = 8;

		const uint32 groupCountX = (device.GetSwapchain().GetWidth() + threadGroupSizeX - 1) / threadGroupSizeX;
		const uint32 groupCountY = (device.GetSwapchain().GetHeight() + threadGroupSizeY - 1) / threadGroupSizeY;

		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

		TransitionVerticalBlurPassImages(commandBuffer);
		constants = { 0, 1 };
		vkCmdPushConstants(commandBuffer, blurPipeline->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blurPipeline->GetPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blurPipeline->GetPipelineLayout(), 0, 1, &verticalBlurDescriptorSets[frameResource.frameSlot], 0, nullptr);

		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

	}

	void Renderer::TransitionPresentPassImages(VkCommandBuffer commandBuffer)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();

		VkImageMemoryBarrier2 barrier = {};
		 barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		 barrier.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		 barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
		 barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		 barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		 barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		 barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		 barrier.image = computeOutputImages[frameResource.frameSlot]->GetImage();
		 barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		 barrier.subresourceRange.baseMipLevel = 0;
		 barrier.subresourceRange.levelCount = 1;
		 barrier.subresourceRange.baseArrayLayer = 0;
		 barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo depInfo = {};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);
	}

	void Renderer::CopyToPresentImage(VkCommandBuffer commandBuffer, VkImage finalImage)
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();

		VkImage presentImage = device.GetCurrentSwapchainImage();

		VkImageMemoryBarrier2 barrier = {};
		
		// 스왑체인 이미지 배리어 
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.image = presentImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo depInfo = {};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		const int32 sourceWidth = static_cast<int32>(computeOutputImages[frameResource.frameSlot]->GetWidth());
		const int32 sourceHeight = static_cast<int32>(computeOutputImages[frameResource.frameSlot]->GetHeight());
		const int32 destinationWidth = static_cast<int32>(device.GetSwapchain().GetWidth());
		const int32 destinationHeight = static_cast<int32>(device.GetSwapchain().GetHeight());

		VkImageBlit blitRegion{};
		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[1] = { sourceWidth, sourceHeight, 1 };

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[1] = { destinationWidth, destinationHeight, 1 };

		vkCmdBlitImage(
			commandBuffer,
			finalImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			presentImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blitRegion,
			VK_FILTER_NEAREST);
	}

	void Renderer::Render()
	{
		const FFrameResource& frameResource = device.GetCurrentFrameResource();
		const VkCommandBuffer commandBuffer = frameResource.commandBuffer;

		GPUProfiler& profiler = device.GetGPUProfiler();
		GPUProfiler::Scope frameScope(profiler, "Render");
		{
			GPUProfiler::Scope scope(profiler, "Scene");
			ScenePass(commandBuffer);
		}
		{
			GPUProfiler::Scope scope(profiler, "Blur");
			BlurPass(commandBuffer);
		}

		// Present
		const VkImage finalImage = computeOutputImages[frameResource.frameSlot]->GetImage();
		GPUProfiler::Scope presentScope(profiler, "Blit");
		TransitionPresentPassImages(commandBuffer);
		// finalImage를 스왑체인 이미지로 변경
		CopyToPresentImage(commandBuffer, finalImage);
	}
}
