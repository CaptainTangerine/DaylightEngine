#pragma once
namespace Dlight
{
    enum class EImageState
    {
        Undefined,

        ColorAttachmentWrite,
        SampledRead,
        StorageRead,
        StorageWrite,

        CopySource,
        CopyDestination,
    };

    enum class EBufferState
    {
        VertexRead,
        IndexRead,
        StorageRead,
        StorageWrite,
        IndirectRead,

        CopySource,
        CopyDestination,
    };

    using RenderGraphResourceState = std::variant<EImageState, EBufferState>;
}
