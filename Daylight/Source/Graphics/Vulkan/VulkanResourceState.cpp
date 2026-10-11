#include "pch.h"
#include "VulkanResourceState.h"

namespace Dlight
{
    FVulkanResourceState ConvertResourceState(EResourceState state)
    {
        switch (state)
        {
        case EResourceState::Undefined:
            return {
                VK_PIPELINE_STAGE_2_NONE,
                VK_ACCESS_2_NONE,
                VK_IMAGE_LAYOUT_UNDEFINED
            };

        case EResourceState::VertexBuffer:
            return {
                VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };

        case EResourceState::IndexBuffer:
            return {
                VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                VK_ACCESS_2_INDEX_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
        case EResourceState::CopyDestination:
            return{
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                 VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
        }
        return {};
    }
}
