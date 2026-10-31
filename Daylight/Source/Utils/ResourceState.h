#pragma once

#include <variant>
#include <vulkan/vulkan.h>

namespace Dlight
{
    enum class EImageState
    {
        Undefined,

        ColorAttachmentWrite,
        DepthStencilAttachmentWrite,
        VertexSampledRead,
        FragmentSampledRead,

        ComputeSampledRead,
        ComputeStorageRead,
        ComputeStorageWrite,

        CopySource,
        CopyDestination,
        Present
    };

    enum class EBufferState
    {
        VertexRead,
        IndexRead,

        FragmentStorageRead,
        FragmentStorageWrite,
        ComputeStorageRead,
        ComputeStorageWrite,

        IndirectRead,

        CopySource,
        CopyDestination,
    };

    struct FTrackedAccess
    {
        VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access = VK_ACCESS_2_NONE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        bool isWrite = false;
        bool hasPreviousAccess = false;
    };

    struct FImageBarrierPlan
    {
        VkPipelineStageFlags2 srcStage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 srcAccess = VK_ACCESS_2_NONE;
        VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 dstAccess = VK_ACCESS_2_NONE;
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct FBufferBarrierPlan
    {
        VkPipelineStageFlags2 srcStage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 srcAccess = VK_ACCESS_2_NONE;

        VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 dstAccess = VK_ACCESS_2_NONE;
    };


    using FRenderGraphBarrierPlan = std::variant<FImageBarrierPlan, FBufferBarrierPlan>;
    using RenderGraphResourceState = std::variant<EImageState, EBufferState>;

    VkImageUsageFlags ToImageUsage(EImageState state);
    VkBufferUsageFlags ToBufferUsage(EBufferState state);
    bool IsWriteState(EImageState state);
    bool IsWriteState(EBufferState state);
    FTrackedAccess ConvertToTrackedAccess(const RenderGraphResourceState& state, bool isWrite);
}
