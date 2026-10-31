#include "pch.h"
#include "BlurPass.h"

#include "Core/Paths.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanImage.h"
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
        resources.resize(VulkanDevice::MaxFramesInFlight);
        CreatePipeline();
        CreateDescriptorSets();
    }

    BlurPass::~BlurPass()
    {
        pipeline.reset();
        DestroyDescriptors();
    }

    void BlurPass::SetResources(uint32 frameSlot, VulkanImage& sceneColor, VulkanImage& horizontalBlur, VulkanImage& output)
    {
        assert(frameSlot < resources.size());
        resources[frameSlot] = { &sceneColor, &horizontalBlur, &output };

        VkDescriptorImageInfo horizontalInputInfo{};
        horizontalInputInfo.imageView = sceneColor.GetImageView();
        horizontalInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo horizontalOutputInfo{};
        horizontalOutputInfo.imageView = horizontalBlur.GetImageView();
        horizontalOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorImageInfo verticalInputInfo{};
        verticalInputInfo.imageView = horizontalBlur.GetImageView();
        verticalInputInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo verticalOutputInfo{};
        verticalOutputInfo.imageView = output.GetImageView();
        verticalOutputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        std::array<VkWriteDescriptorSet, 4> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = horizontalDescriptorSets[frameSlot];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageInfo = &horizontalInputInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = horizontalDescriptorSets[frameSlot];
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].pImageInfo = &horizontalOutputInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = verticalDescriptorSets[frameSlot];
        writes[2].dstBinding = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[2].pImageInfo = &verticalInputInfo;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = verticalDescriptorSets[frameSlot];
        writes[3].dstBinding = 1;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[3].pImageInfo = &verticalOutputInfo;

        vkUpdateDescriptorSets(device.GetDevice(), static_cast<uint32>(writes.size()), writes.data(), 0, nullptr);
    }

    void BlurPass::Execute(VkCommandBuffer commandBuffer, uint32 frameSlot, VkExtent2D extent)
    {
        assert(frameSlot < resources.size());
        assert(resources[frameSlot].sceneColor);
        assert(resources[frameSlot].horizontalBlur);
        assert(resources[frameSlot].output);

        TransitionHorizontalImages(commandBuffer, frameSlot);

        FBlurPushConstants constants{ { 1, 0 } };
        vkCmdPushConstants(commandBuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipelineLayout(), 0, 1, &horizontalDescriptorSets[frameSlot], 0, nullptr);

        constexpr uint32 threadGroupSizeX = 8;
        constexpr uint32 threadGroupSizeY = 8;
        const uint32 groupCountX = (extent.width + threadGroupSizeX - 1) / threadGroupSizeX;
        const uint32 groupCountY = (extent.height + threadGroupSizeY - 1) / threadGroupSizeY;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);

        TransitionVerticalImages(commandBuffer, frameSlot);

        constants = { 0, 1 };
        vkCmdPushConstants(commandBuffer, pipeline->GetPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipelineLayout(), 0, 1, &verticalDescriptorSets[frameSlot], 0, nullptr);
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
        constexpr uint32 frameCount = VulkanDevice::MaxFramesInFlight;
        constexpr uint32 descriptorSetCount = frameCount * 2;

        horizontalDescriptorSets.resize(frameCount);
        verticalDescriptorSets.resize(frameCount);

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

        std::array<VkDescriptorSetLayout, frameCount> layouts;
        layouts.fill(descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = frameCount;
        allocateInfo.pSetLayouts = layouts.data();

        if (VK_SUCCESS != vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, horizontalDescriptorSets.data()))
        {
            DL_LOG_ERROR("Failed to allocate horizontal blur descriptor sets");
            std::abort();
        }

        if (VK_SUCCESS != vkAllocateDescriptorSets(device.GetDevice(), &allocateInfo, verticalDescriptorSets.data()))
        {
            DL_LOG_ERROR("Failed to allocate vertical blur descriptor sets");
            std::abort();
        }
    }

    void BlurPass::DestroyDescriptors()
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device.GetDevice(), descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }

        horizontalDescriptorSets.clear();
        verticalDescriptorSets.clear();

        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device.GetDevice(), descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    void BlurPass::TransitionHorizontalImages(VkCommandBuffer commandBuffer, uint32 frameSlot)
    {
        const FResources& frameResources = resources[frameSlot];

        VkImageMemoryBarrier2 inputBarrier{};
        inputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        inputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        inputBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        inputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        inputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        inputBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        inputBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputBarrier.image = frameResources.sceneColor->GetImage();
        inputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        inputBarrier.subresourceRange.levelCount = 1;
        inputBarrier.subresourceRange.layerCount = 1;

        VkDependencyInfo inputDependency{};
        inputDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        inputDependency.imageMemoryBarrierCount = 1;
        inputDependency.pImageMemoryBarriers = &inputBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &inputDependency);

        VkImageMemoryBarrier2 outputBarrier{};
        outputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        outputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        outputBarrier.srcAccessMask = VK_ACCESS_2_NONE;
        outputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        outputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        outputBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        outputBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        outputBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        outputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        outputBarrier.image = frameResources.horizontalBlur->GetImage();
        outputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        outputBarrier.subresourceRange.levelCount = 1;
        outputBarrier.subresourceRange.layerCount = 1;

        VkDependencyInfo outputDependency{};
        outputDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        outputDependency.imageMemoryBarrierCount = 1;
        outputDependency.pImageMemoryBarriers = &outputBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &outputDependency);
    }

    void BlurPass::TransitionVerticalImages(VkCommandBuffer commandBuffer, uint32 frameSlot)
    {
        const FResources& frameResources = resources[frameSlot];

        VkImageMemoryBarrier2 inputBarrier{};
        inputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        inputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        inputBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
        inputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        inputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        inputBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        inputBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputBarrier.image = frameResources.horizontalBlur->GetImage();
        inputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        inputBarrier.subresourceRange.levelCount = 1;
        inputBarrier.subresourceRange.layerCount = 1;

        VkDependencyInfo inputDependency{};
        inputDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        inputDependency.imageMemoryBarrierCount = 1;
        inputDependency.pImageMemoryBarriers = &inputBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &inputDependency);

        VkImageMemoryBarrier2 outputBarrier{};
        outputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        outputBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        outputBarrier.srcAccessMask = VK_ACCESS_2_NONE;
        outputBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        outputBarrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
        outputBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        outputBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        outputBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        outputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        outputBarrier.image = frameResources.output->GetImage();
        outputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        outputBarrier.subresourceRange.levelCount = 1;
        outputBarrier.subresourceRange.layerCount = 1;

        VkDependencyInfo outputDependency{};
        outputDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        outputDependency.imageMemoryBarrierCount = 1;
        outputDependency.pImageMemoryBarriers = &outputBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &outputDependency);
    }
}
