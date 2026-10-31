#pragma once

#include "CoreTypes.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace Dlight
{
    class ResourceManager;
    class ShaderCompiler;
    class VulkanComputePipeline;
    class VulkanDevice;
    class VulkanImage;

    class BlurPass
    {
    public:
        BlurPass(VulkanDevice& device, ResourceManager& resourceManager, ShaderCompiler& shaderCompiler);
        ~BlurPass();

        BlurPass(const BlurPass&) = delete;
        BlurPass& operator=(const BlurPass&) = delete;

        void SetResources(uint32 frameSlot, VulkanImage& sceneColor, VulkanImage& horizontalBlur, VulkanImage& output);
        void Execute(VkCommandBuffer commandBuffer, uint32 frameSlot, VkExtent2D extent);

    private:
        struct FResources
        {
            VulkanImage* sceneColor = nullptr;
            VulkanImage* horizontalBlur = nullptr;
            VulkanImage* output = nullptr;
        };

        void CreatePipeline();
        void CreateDescriptorSets();
        void DestroyDescriptors();
        void TransitionHorizontalImages(VkCommandBuffer commandBuffer, uint32 frameSlot);
        void TransitionVerticalImages(VkCommandBuffer commandBuffer, uint32 frameSlot);

        VulkanDevice& device;
        ResourceManager& resourceManager;
        ShaderCompiler& shaderCompiler;

        std::unique_ptr<VulkanComputePipeline> pipeline;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> horizontalDescriptorSets;
        std::vector<VkDescriptorSet> verticalDescriptorSets;
        std::vector<FResources> resources;
    };
}
