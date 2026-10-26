#pragma once
#include <limits>

namespace Dlight
{
    class RenderGraphHandle
    {
    private:
        RenderGraphHandle() noexcept = default;
        explicit RenderGraphHandle(uint32 const index) noexcept : index(index) {}

    public:
        RenderGraphHandle(RenderGraphHandle const& rhs) noexcept = default;
        RenderGraphHandle& operator=(RenderGraphHandle const& rhs) noexcept = default;

        bool isInitialized() const noexcept { return index != UNINITIALIZED; }

        uint32 getIndex() const noexcept { return index; }

        operator bool() const noexcept { return isInitialized(); }

        void clear() noexcept { index = UNINITIALIZED; }

        bool operator < (const RenderGraphHandle& rhs) const noexcept
        {
            return index < rhs.index;
        }

        bool operator == (const RenderGraphHandle& rhs) const noexcept
        {
            return (index == rhs.index);
        }

        bool operator != (const RenderGraphHandle& rhs) const noexcept
        {
            return !operator==(rhs);
        }
    private:
        // index to the resource handle
        static constexpr uint32 UNINITIALIZED = std::numeric_limits<uint32>::max();
        uint32 index = UNINITIALIZED;
    };

    template<typename T>
    class RenderGraphID : RenderGraphHandle
    {
    public:
        using RenderGraphHandle::RenderGraphHandle;
        RenderGraphID() noexcept = default;
        explicit RenderGraphID(const RenderGraphHandle rhs) : RenderGraphHandle(rhs) {};
    };
}
