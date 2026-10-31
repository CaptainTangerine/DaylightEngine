#include "pch.h"
#include "ResourceState.h"

#include <cassert>

namespace Dlight
{
    VkImageUsageFlags ToImageUsage(EImageState state)
    {
        switch (state)
        {
        case EImageState::ColorAttachmentWrite:
            return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        case EImageState::DepthStencilAttachmentWrite:
            return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        case EImageState::VertexSampledRead:
        case EImageState::FragmentSampledRead:
        case EImageState::ComputeSampledRead:
            return VK_IMAGE_USAGE_SAMPLED_BIT;

        case EImageState::ComputeStorageRead:
        case EImageState::ComputeStorageWrite:
            return VK_IMAGE_USAGE_STORAGE_BIT;

        case EImageState::CopySource:
            return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        case EImageState::CopyDestination:
            return VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        case EImageState::Undefined:
        case EImageState::Present:
            return 0;

        default:
            assert(false && "Invalid image state");
            return 0;
        }
    }

    VkBufferUsageFlags ToBufferUsage(EBufferState state)
    {
        switch (state)
        {
        case EBufferState::VertexRead:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        case EBufferState::IndexRead:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        case EBufferState::FragmentStorageRead:
        case EBufferState::FragmentStorageWrite:
        case EBufferState::ComputeStorageRead:
        case EBufferState::ComputeStorageWrite:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        case EBufferState::IndirectRead:
            return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

        case EBufferState::CopySource:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        case EBufferState::CopyDestination:
            return VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        default:
            assert(false && "Invalid buffer state");
            return 0;
        }
    }

    bool IsWriteState(EImageState state)
    {
        switch (state)
        {
        case EImageState::ColorAttachmentWrite:
        case EImageState::DepthStencilAttachmentWrite:
        case EImageState::ComputeStorageWrite:
        case EImageState::CopyDestination:
            return true;

        default:
            return false;
        }
    }

    bool IsWriteState(EBufferState state)
    {
        switch (state)
        {
        case EBufferState::FragmentStorageWrite:
        case EBufferState::ComputeStorageWrite:
        case EBufferState::CopyDestination:
            return true;

        default:
            return false;
        }
    }

    FTrackedAccess ConvertToTrackedAccess(const RenderGraphResourceState& state, bool isWrite)
    {
        FTrackedAccess tracked = {};
        tracked.isWrite = isWrite;
        tracked.hasPreviousAccess = true;

        if (const EImageState* imageState = std::get_if<EImageState>(&state))
        {
            switch (*imageState)
            {
            case EImageState::Undefined:
                break;

            case EImageState::ColorAttachmentWrite:
                tracked.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                tracked.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;

            case EImageState::DepthStencilAttachmentWrite:
                tracked.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                tracked.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                break;

            case EImageState::VertexSampledRead:
                tracked.stage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
                tracked.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;

            case EImageState::FragmentSampledRead:
                tracked.stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                tracked.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;

            case EImageState::ComputeSampledRead:
                tracked.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                tracked.access = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;

            case EImageState::ComputeStorageRead:
                tracked.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                tracked.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_GENERAL;
                break;

            case EImageState::ComputeStorageWrite:
                tracked.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
                tracked.access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_GENERAL;
                break;

            case EImageState::CopySource:
                tracked.stage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
                tracked.access = VK_ACCESS_2_TRANSFER_READ_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                break;

            case EImageState::CopyDestination:
                tracked.stage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
                tracked.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                tracked.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;

            case EImageState::Present:
                tracked.stage = VK_PIPELINE_STAGE_2_NONE;
                tracked.access = VK_ACCESS_2_NONE;
                tracked.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                break;
            }

            return tracked;
        }

        const EBufferState* bufferState = std::get_if<EBufferState>(&state);
        if (!bufferState)
        {
            return tracked;
        }

        switch (*bufferState)
        {
        case EBufferState::VertexRead:
            tracked.stage = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
            tracked.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
            break;

        case EBufferState::IndexRead:
            tracked.stage = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
            tracked.access = VK_ACCESS_2_INDEX_READ_BIT;
            break;

        case EBufferState::FragmentStorageRead:
            tracked.stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            tracked.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
            break;

        case EBufferState::FragmentStorageWrite:
            tracked.stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            tracked.access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
            break;

        case EBufferState::ComputeStorageRead:
            tracked.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            tracked.access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
            break;

        case EBufferState::ComputeStorageWrite:
            tracked.stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            tracked.access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
            break;

        case EBufferState::IndirectRead:
            tracked.stage = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
            tracked.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
            break;

        case EBufferState::CopySource:
            tracked.stage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
            tracked.access = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;

        case EBufferState::CopyDestination:
            tracked.stage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
            tracked.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        }

        return tracked;
    }
}
