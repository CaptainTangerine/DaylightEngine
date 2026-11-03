#include "pch.h"
#include "BlurPass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Rendering/PushConstants.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderConventions.h"
#include "Resources/ResourceManager.h"

namespace Dlight
{
    BlurPass::BlurPass(VulkanDevice& _device, ResourceManager& _resourceManager, ShaderCompiler& _shaderCompiler)
        : device(_device)
        , resourceManager(_resourceManager)
        , shaderCompiler(_shaderCompiler)
    {
        CreatePipeline();
        CreateDescriptorSets();
    }

    BlurPass::~BlurPass()
    {
        pipeline.reset();
        DestroyDescriptors();
    }

    void BlurPass::SetResources(VkImageView sceneColor, VkImageView horizontalBlur, VkImageView output)
    {
        VkDescriptorImageInfo horizontalInputInfo{};
        horizontalInputInfo.imageView = sceneColor;
        horizontalInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo horizontalOutputInfo{};
        horizontalOutputInfo.imageView = horizontalBlur;
        horizontalOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorImageInfo verticalInputInfo{};
        verticalInputInfo.imageView = horizontalBlur;
        verticalInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo verticalOutputInfo{};
        verticalOutputInfo.imageView = output;
        verticalOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        std::array<VkWriteDescriptorSet, 4> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = horizontalDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageInfo = &horizontalInputInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = horizontalDescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageInfo = &horizontalOutputInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = verticalDescriptorSet;
        writes[2].dstBinding = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2].pImageInfo = &verticalInputInfo;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = verticalDescriptorSet;
        writes[3].dstBinding = 1;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[3].pImageInfo = &verticalOutputInfo;

        vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
    }

    void BlurPass::ExecuteHorizontal(VkCommandBuffer commandBuffer, VkExtent2D extent)
    {
        FBlurPushConstants constants{ { 1, 0 } };
        vkCmdPushConstants(commandBuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipelineLayout(), 0, 1, &horizontalDescriptorSet, 0, nullptr);

        constexpr uint32 threadGroupSizeX = 8;
        constexpr uint32 threadGroupSizeY = 8;
        const uint32 groupCountX = (extent.width + threadGroupSizeX - 1) / threadGroupSizeX;
        const uint32 groupCountY = (extent.height + threadGroupSizeY - 1) / threadGroupSizeY;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    }

    void BlurPass::ExecuteVertical(VkCommandBuffer commandBuffer, VkExtent2D extent)
    {
        FBlurPushConstants constants{ { 0, 1 } };
        vkCmdPushConstants(commandBuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipelineLayout(), 0, 1, &verticalDescriptorSet, 0, nullptr);

        constexpr uint32 threadGroupSizeX = 8;
        constexpr uint32 threadGroupSizeY = 8;
        const uint32 groupCountX = (extent.width + threadGroupSizeX - 1) / threadGroupSizeX;
        const uint32 groupCountY = (extent.height + threadGroupSizeY - 1) / threadGroupSizeY;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    }

    void BlurPass::CreatePipeline()
    {
        const std::filesystem::path shaderPath = Paths::GetShaderPath(L"Blur.hlsl");
        const std::vector<uint32> compCode = shaderCompiler.Compile(shaderPath.c_str(), ShaderConventions::ComputeEntry, L"cs_6_0");

        VkShaderModule compModule = VK_NULL_HANDLE;
        VkShaderModuleCreateInfo compModuleInfo{};
        compModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        compModuleInfo.codeSize = compCode.size() * sizeof(uint32);
        compModuleInfo.pCode = compCode.data();

        if (VK_SUCCESS != vkCreateShaderModule(device.GetDevice(), &compModuleInfo, nullptr, &compModule))
        {
            DL_LOG_ERROR("Failed to create blur compute shader module");
            std::abort();
        }

        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (VK_SUCCESS != vkCreateDescriptorSetLayout(device.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout))
        {
            vkDestroyShaderModule(device.GetDevice(), compModule, nullptr);
            DL_LOG_ERROR("Failed to create blur descriptor set layout");
            std::abort();
        }

        FComputePipelineDesc pipelineDesc{};
        pipelineDesc.computeShader = compModule;
        pipelineDesc.computeEntryPoint = ShaderConventions::ComputeEntry;
        pipelineDesc.descriptorSetLayouts = { descriptorSetLayout };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(FBlurPushConstants);
        pipelineDesc.pushConstantRanges = { pushConstantRange };

        pipeline = std::make_unique<VulkanComputePipeline>(device, pipelineDesc);
        vkDestroyShaderModule(device.GetDevice(), compModule, nullptr);
    }

    void BlurPass::CreateDescriptorSets()
    {
        constexpr uint32 descriptorSetCount = 2;

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].descriptorCount = descriptorSetCount;
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[1].descriptorCount = descriptorSetCount;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = descriptorSetCount;
        poolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        if (VK_SUCCESS != vkCreateDescriptorPool(device.GetDevice(), &poolInfo, nullptr, &descriptorPool))
        {
            DL_LOG_ERROR("Failed to create blur descriptor pool");
            std::abort();
        }

        std::array<VkDescriptorSetLayout, descriptorSetCount> layouts;
        layouts.fill(descriptorSetLayout);

        std::array<VkDescriptorSet, descriptorSetCount> descriptorSets = {};

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = descriptorSetCount;
        allocateInfo.pSetLayouts = layouts.data();

        if (VK_SUCCESS != vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, descriptorSets.data()))
        {
            DL_LOG_ERROR("Failed to allocate blur descriptor sets");
            std::abort();
        }

        horizontalDescriptorSet = descriptorSets[0];
        verticalDescriptorSet = descriptorSets[1];
    }

    void BlurPass::DestroyDescriptors()
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device.GetDevice(), descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }

        horizontalDescriptorSet = VK_NULL_HANDLE;
        verticalDescriptorSet = VK_NULL_HANDLE;

        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device.GetDevice(), descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

}
