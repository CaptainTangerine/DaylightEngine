#pragma once

#include "CoreTypes.h"

#include <memory>
#include <vulkan/vulkan.h>

namespace Dlight
{
    class ResourceManager;
    class ShaderCompiler;
    class VulkanComputePipeline;
    class VulkanDevice;

    class BlurPass
    {
    public:
        BlurPass(VulkanDevice& device, ResourceManager& resourceManager, ShaderCompiler& shaderCompiler);
        ~BlurPass();

        BlurPass(const BlurPass&) = delete;
        BlurPass& operator=(const BlurPass&) = delete;

        void SetResources(VkImageView sceneColor, VkImageView horizontalBlur, VkImageView output);
        void ExecuteHorizontal(VkCommandBuffer commandBuffer, VkExtent2D extent);
        void ExecuteVertical(VkCommandBuffer commandBuffer, VkExtent2D extent);

    private:
        void CreatePipeline();
        void CreateDescriptorSets();
        void DestroyDescriptors();

        VulkanDevice& device;
        ResourceManager& resourceManager;
        ShaderCompiler& shaderCompiler;

        std::unique_ptr<VulkanComputePipeline> pipeline;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet horizontalDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet verticalDescriptorSet = VK_NULL_HANDLE;
    };
}
