#pragma once
#include "Resource.h"
#include <vulkan/vulkan.h>

namespace Dlight
{
    class VulkanDevice;

    struct FShaderRaw
    {
        std::vector<uint32> spirv;
        VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
        std::string entryPoint;
    };

    class ShaderResource : public Resource
    {
    public:
        ShaderResource(const std::string& id, VulkanDevice& _device, FShaderRaw _shaderRaw);
        ~ShaderResource() override;

        ShaderResource(const ShaderResource&) = delete;
        ShaderResource& operator=(const ShaderResource&) = delete;

        VkShaderModule GetModule() const { return shaderModule; }
        VkShaderStageFlagBits GetStage() const { return shaderRaw.stage; }
        const std::string& GetEntryPoint() const { return shaderRaw.entryPoint; }

    protected:
        bool DoLoad() override;
        void DoUnload() override;

    private:
        VulkanDevice& device;
        FShaderRaw shaderRaw;

        VkShaderModule shaderModule = { VK_NULL_HANDLE };
    };
}
