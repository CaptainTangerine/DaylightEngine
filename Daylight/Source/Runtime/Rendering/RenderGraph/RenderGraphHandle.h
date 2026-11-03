#pragma once

#include "CoreTypes.h"
#include <limits>

namespace Dlight
{
    enum class ERenderGraphResourceType
    {
        Image,
        Buffer
    };

    struct RenderGraphResourceID
    {
        ERenderGraphResourceType type;
        uint32 index = std::numeric_limits<uint32>::max();

        bool operator<(RenderGraphResourceID const& rhs) const noexcept
        {
            if (type != rhs.type)
            {
                return type < rhs.type;
            }

            return index < rhs.index;
        }
    };

    template<typename RESOURCE>
    class RenderGraphHandle
    {
    public:
        RenderGraphHandle() noexcept = default;
        RenderGraphHandle(RenderGraphHandle const&) noexcept = default;
        RenderGraphHandle& operator=(RenderGraphHandle const&) noexcept = default;

        bool IsInitialized() const noexcept
        {
            return id.index != UNINITIALIZED;
        }

        explicit operator bool() const noexcept
        {
            return IsInitialized();
        }

        RenderGraphResourceID const& GetID() const noexcept
        {
            return id;
        }

        void Clear() noexcept
        {
            id.index = UNINITIALIZED;
        }

    private:
        friend class RenderGraph;

        explicit RenderGraphHandle(RenderGraphResourceID resourceID) noexcept
            : id(resourceID)
        {
        }

        static constexpr uint32 UNINITIALIZED = std::numeric_limits<uint32>::max();
        RenderGraphResourceID id = { };
    };

    struct RenderGraphImageTag {};
    struct RenderGraphBufferTag {};
    using RenderGraphImageHandle = RenderGraphHandle<RenderGraphImageTag>;
    using RenderGraphBufferHandle = RenderGraphHandle<RenderGraphBufferTag>;
}
