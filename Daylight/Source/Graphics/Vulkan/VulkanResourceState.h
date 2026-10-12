#pragma once
#include <vulkan/vulkan.h>

namespace Dlight
{
    enum class EResourceState
    {
        Undefined,

        VertexBuffer,
        IndexBuffer,

        CopyDestination,

        FragmentShaderRead,
    };

    struct FVulkanResourceState
    {
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;

        VkAccessFlags2 access = VK_ACCESS_2_NONE;

        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    FVulkanResourceState ConvertResourceState(EResourceState state);
}
