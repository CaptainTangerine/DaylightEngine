#include "pch.h"
#include "ShaderResource.h"
#include "Graphics/Vulkan/VulkanDevice.h"

namespace Dlight
{
    ShaderResource::ShaderResource(const std::string& id, VulkanDevice& _device, FShaderRaw _shaderRaw)
        : Resource(id)
        , device(_device)
        , shaderRaw(std::move(_shaderRaw))
    {
    }

    ShaderResource::~ShaderResource()
    {
        DoUnload();
    }

    bool ShaderResource::DoLoad()
    {
        if (IsLoaded())
        {
            return true;
        }

        if (shaderRaw.spirv.empty() || shaderRaw.entryPoint.empty())
        {
            DL_LOG_ERROR("Invalid shader data: ", GetID());
            return false;
        }

        VkShaderModuleCreateInfo moduleInfo = {};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = shaderRaw.spirv.size() * sizeof(uint32);
        moduleInfo.pCode = shaderRaw.spirv.data();

        if (VK_SUCCESS != vkCreateShaderModule(device.GetDevice(), &moduleInfo, nullptr, &shaderModule))
        {
            shaderModule = VK_NULL_HANDLE;
            DL_LOG_ERROR("Failed to create shader module: ", GetID());
            return false;
        }

        return true;
    }

    void ShaderResource::DoUnload()
    {
        if (shaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(device.GetDevice(), shaderModule, nullptr);
            shaderModule = VK_NULL_HANDLE;
        }
    }
}
