#include "pch.h"
#include "Renderer.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Graphics/Vulkan/VulkanPipeline.h"

#include "ShaderConventions.h"

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
		if (!CreateDepthStencilResources())
		{
			DL_LOG_ERROR("Failed to create depth-stencil resources");
			std::abort();
		}

		CreatePipelines();
	}

	void Renderer::Shutdown()
	{
		DestroyPipelines();

		DestroyDepthStencilResources();
	}

	bool Renderer::CreateDepthStencilResources()
	{
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), depthStencilFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			DL_LOG_ERROR("VK_FORMAT_D32_SFLOAT_S8_UINT is not supported as a depth-stencil attachment");
			return false;
		}

		DestroyDepthStencilResources();

		VkImageCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthStencilCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthStencilCreateInfo.format = depthStencilFormat;
		depthStencilCreateInfo.extent.width = device.GetSwapchain().GetWidth();
		depthStencilCreateInfo.extent.height = device.GetSwapchain().GetHeight();
		depthStencilCreateInfo.extent.depth = 1;
		depthStencilCreateInfo.mipLevels = 1;
		depthStencilCreateInfo.arrayLayers = 1;
		depthStencilCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthStencilCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthStencilCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthStencilCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (VK_SUCCESS != vmaCreateImage(device.GetVmaAllocator(), &depthStencilCreateInfo, &allocInfo, &depthStencilImage, &depthStencilImageAllocation, nullptr))
		{
			DL_LOG_ERROR("Failed to allocate depth-stencil image");
			return false;
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthStencilImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthStencilFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &depthStencilImageView) != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to create depth-stencil image view");
			DestroyDepthStencilResources();
			return false;
		}

		return true;
	}

	void Renderer::DestroyDepthStencilResources()
	{
		if (depthStencilImageView)
		{
			vkDestroyImageView(device.GetDevice(), depthStencilImageView, nullptr);
			depthStencilImageView = nullptr;
		}
		if (depthStencilImage)
		{
			vmaDestroyImage(device.GetVmaAllocator(), depthStencilImage, depthStencilImageAllocation);
			depthStencilImage = nullptr;
			depthStencilImageAllocation = nullptr;
		}
	}

	bool Renderer::CreatePipelines()
	{
		// 셰이더 컴파일 -> 추후 shaderManager로 관리 
		auto vertCode = shaderCompiler.Compile(
			L"Shaders/Triangle.hlsl",
			ShaderConventions::VertexEntry,
			L"vs_6_0");

		auto fragCode = shaderCompiler.Compile(
			L"Shaders/Triangle.hlsl",
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


		GfxPipelineDesc desc{};
		desc.vertexShader = vertModule;
		desc.fragmentShader = fragModule;
		desc.vertexEntryPoint = ShaderConventions::VertexEntry;
		desc.fragmentEntryPoint = ShaderConventions::PixelEntry;
		desc.colorFormat = VulkanSwapchain::swapchainFormat;
		desc.depthFormat = depthStencilFormat;

		trianglePipeline = std::make_unique<VulkanPipeline>(device, desc);

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
	}

}
