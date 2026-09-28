#include "pch.h"
#include "VulkanPipeline.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Rendering/Renderer.h"

Dlight::VulkanPipeline::VulkanPipeline(VulkanDevice& _device, const GfxPipelineDesc& desc)
	: device(_device)
{
	Initialize(desc);
}

Dlight::VulkanPipeline::~VulkanPipeline()
{
	Shutdown();
}

void Dlight::VulkanPipeline::Initialize(const GfxPipelineDesc& desc)
{
	// vkPipelineLayout
	// 디스크립터나 push Constants 정보 
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (VK_SUCCESS != vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout))
	{
		DL_LOG_ERROR("Fail to create pipelinelayout");
		std::abort();
	}
	// vkPipieline 
	// 0. ShaderStage
	// 1. VertexInput(inputLayout이랑 비슷)
	// 2. InputAssembly 
	// 3. Veiewport / Cissor
	// 4. 레스터라이제이션
	// 5. 멀티샘플링
	// 6. Depth/stencil
	// 7. blend
	// 8. Dynamic State

	// 0
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	stages.resize(2);

	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = desc.vertexShader;
	stages[0].pName = desc.vertexEntryPoint;

	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = desc.fragmentShader;
	stages[1].pName = desc.fragmentEntryPoint;
	
	// 1
	VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
	vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	// 2 
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// 3
	VkPipelineViewportStateCreateInfo viewportInfo = {};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = nullptr;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = nullptr;
	
	// 4 
	VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationInfo.lineWidth = 1.f;

	// 5
	VkPipelineMultisampleStateCreateInfo multiSamplingInfo = {};
	multiSamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// no multisampling
	multiSamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// 6
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	// 7
	VkPipelineColorBlendAttachmentState attachState = {};
	attachState.blendEnable = VK_FALSE;
	attachState.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blendInfo = {};
	blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = &attachState;

	// 8
	std::vector<VkDynamicState> dynamicState{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = static_cast<uint32>(dynamicState.size());
	dynamicStateInfo.pDynamicStates = dynamicState.data();

	VkPipelineRenderingCreateInfo renderInfo = {};
	renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachmentFormats = &desc.colorFormat;
	renderInfo.depthAttachmentFormat = desc.depthFormat;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &renderInfo;
	pipelineInfo.stageCount = static_cast<uint32>(stages.size());
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizationInfo;
	pipelineInfo.pMultisampleState = &multiSamplingInfo;
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.pColorBlendState = &blendInfo;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = pipelineLayout;
	
	// use dynamicRendering 
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	if (VK_SUCCESS != vkCreateGraphicsPipelines(device.GetDevice(), nullptr, 1, &pipelineInfo, nullptr, &pipeline))
	{
		DL_LOG_ERROR("Error Createing the pipeline");
		std::abort();
	}
}

void Dlight::VulkanPipeline::Shutdown()
{
	if (pipeline)
	{
		vkDestroyPipeline(device.GetDevice(), pipeline, nullptr);
	}

	if (pipelineLayout)
	{
		vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr);
	}
}
