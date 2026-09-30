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
		CreatePipelines();
	}

	void Renderer::Shutdown()
	{
		DestroyPipelines();
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
		desc.depthFormat = device.GetDepthStencilFormat();

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

	void Renderer::Render()
	{
	}

}
