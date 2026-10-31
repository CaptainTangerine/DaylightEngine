#pragma once
#include "vulkan/vulkan.h"
#include "RenderGraphHandle.h"
#include "Utils/ResourceState.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include <optional>

namespace Dlight
{
    // 렌더 그래프는 패스간의 의존성을 관리한다.
    // 복잡하게 하지 않고 노드를 Resource와 Pass로 나누지 않고 Pass로 둠
    // Memory Aliasing은 추후 과제로 남겨두고, 각 Resource의 의존성 관리 Pass 추가의 확장성, 유지보수를 목적으로 한다.
    // 렌더 그래프의 리소스들은 대부분 이미지이긴 하지만 확장 가능성을 열어둠

    class RenderGraphPass;
    class VulkanDevice;

    struct FInternalRenderGraphTexture
    {
        FVulkanImageDesc description;
        std::unique_ptr<VulkanImage> physicalResource;
    };

    struct FExternalRenderGraphTexture
    {
        FVulkanImageDesc description;
        VkImage physicalResource = VK_NULL_HANDLE;
        EImageState initialState = EImageState::Undefined;
        std::optional<EImageState> finalState;
    };

    struct FInternalRenderGraphBuffer
    {
        FVulkanBufferDesc description;
        std::unique_ptr<VulkanBuffer> physicalResource;
    };

    struct FExternalRenderGraphBuffer
    {
        FVulkanBufferDesc description;
        VkBuffer physicalResource = VK_NULL_HANDLE;
        EBufferState initialState;
        std::optional<EBufferState> finalState;
    };

    using RenderGraphTextureResource = std::variant<FInternalRenderGraphTexture, FExternalRenderGraphTexture>;
    using RenderGraphBufferResource = std::variant<FInternalRenderGraphBuffer, FExternalRenderGraphBuffer>;
    using RenderGraphResource = std::variant<RenderGraphTextureResource, RenderGraphBufferResource>;

    struct PassNode
    {
        std::string name;
        std::map<RenderGraphResourceID, RenderGraphResourceState> reads;
        std::map<RenderGraphResourceID, RenderGraphResourceState> writes;
        std::map<RenderGraphResourceID, FRenderGraphBarrierPlan> barriers;

        std::function<void(VkCommandBuffer)> execFunc;
    };

    class RenderGraph
    {
    public:
        class Builder
        {
        private:
            friend class RenderGraph;
            Builder(RenderGraph& rg, PassNode& _passNode) noexcept;
            ~Builder() noexcept = default;

        public:
            Builder(Builder const&) = delete;
            Builder& operator=(Builder const&) = delete;

            // 하나의 PassNode를 선언
            RenderGraphImageHandle Read(RenderGraphImageHandle resource, EImageState usage);
            RenderGraphImageHandle Write(RenderGraphImageHandle resource, EImageState usage);

            RenderGraphBufferHandle Read(RenderGraphBufferHandle resource, EBufferState usage);
            RenderGraphBufferHandle Write(RenderGraphBufferHandle resource, EBufferState usage);

            // 리소스 선언
            RenderGraphImageHandle  DeclareInternal(FVulkanImageDesc image);
            RenderGraphBufferHandle DeclareInternal(FVulkanBufferDesc buffer);

            // 완성된 외부 리소스
            RenderGraphImageHandle DeclareExternal(FVulkanImageDesc desc, EImageState initialState, std::optional<EImageState> finalState = std::nullopt);
            RenderGraphBufferHandle DeclareExternal(FVulkanBufferDesc desc, EBufferState initialState, std::optional<EBufferState> finalState = std::nullopt);

        private:
            RenderGraph& renderGraph;
            PassNode& passNode;
        };

        RenderGraph(VulkanDevice& _device) noexcept;
        ~RenderGraph() noexcept;

        RenderGraph(RenderGraph const&) = delete;
        RenderGraph& operator=(RenderGraph const&) = delete;
    public:
        // RenderGraph API
        template<typename Data, typename SetupFunc, typename ExecuteFunc>
        Data AddPass(const std::string& passName, SetupFunc&& setup, ExecuteFunc&& execute);

        // Compile
        // Usage 중첩 / 리소스 생성 / TODO : PassCulling, 순환구조 탐색, 위상정렬
        void Compile();

        void BindExternal(RenderGraphImageHandle handle, VkImage externalResource);
        void BindExternal(RenderGraphBufferHandle handle, VkBuffer externalResource);

        void Execute(VkCommandBuffer commandBuffer);

    private:
        PassNode& CreateRenderPass(const std::string& passName);
        RenderGraphImageHandle DeclareInternalResource(FVulkanImageDesc desc);
        RenderGraphBufferHandle DeclareInternalResource(FVulkanBufferDesc desc);

        RenderGraphImageHandle DeclareExternalResource(FVulkanImageDesc desc, EImageState initialState, std::optional<EImageState> finalState);
        RenderGraphBufferHandle DeclareExternalResource(FVulkanBufferDesc desc, EBufferState initialState, std::optional<EBufferState> finalState);

        void AccumulateUsage(const RenderGraphResourceID& id, const RenderGraphResourceState& state);
        FVulkanImageDesc& GetTextureDescription(RenderGraphTextureResource& resource);
        const FVulkanImageDesc& GetTextureDescription(const RenderGraphTextureResource& resource) const;
        VkImage GetPhysicalImage(const RenderGraphTextureResource& resource) const;

        FVulkanBufferDesc& GetBufferDescription(RenderGraphBufferResource& resource);
        const FVulkanBufferDesc& GetBufferDescription(const RenderGraphBufferResource& resource) const;
        VkBuffer GetPhysicalBuffer(const RenderGraphBufferResource& resource) const;

        void CreatePhysicalResource(FInternalRenderGraphTexture& resource);
        void CreatePhysicalResource(FInternalRenderGraphBuffer& resource);

        FRenderGraphBarrierPlan MakeBarrierPlan(const RenderGraphResourceID& id, const FTrackedAccess& previous, const FTrackedAccess& current);
        bool NeedsBarrier(const RenderGraphResourceID& id, const FTrackedAccess& previous, const FTrackedAccess& current);
        void InitializeExternalAccesses(std::vector<FTrackedAccess>& lastAccesses);
        void CreateExternalFinalBarriers(const std::vector<FTrackedAccess>& lastAccesses);
        void CreateResourceBarrier();

        void RecordBarrier(VkCommandBuffer commandBuffer, RenderGraphResourceID id, const FImageBarrierPlan& barrierPlan);
        void RecordBarrier(VkCommandBuffer commandBuffer, RenderGraphResourceID id, const FBufferBarrierPlan& barrierPlan);
        void RecordBarriers(VkCommandBuffer commandBuffer, const std::map<RenderGraphResourceID, FRenderGraphBarrierPlan>& barriers);
    private:
        VulkanDevice& device;

        std::vector<std::unique_ptr<PassNode>> passes;
        std::vector<RenderGraphResource> resources;

        std::map<RenderGraphResourceID, FRenderGraphBarrierPlan> initialBarriers;
        std::map<RenderGraphResourceID, FRenderGraphBarrierPlan> frameStartBarriers;
        std::map<RenderGraphResourceID, FRenderGraphBarrierPlan> finalBarriers;

        bool hasExecuted = false;
    };

    template<typename Data, typename SetupFunc, typename ExecuteFunc>
    Data RenderGraph::AddPass(const std::string& passName, SetupFunc&& setup, ExecuteFunc&& execute)
    {
        PassNode& pass = CreateRenderPass(passName);

        Data data = {};
        Builder builder(*this, pass);

        // SetUp 호출 시 수행
        setup(builder, data);

        // Execute 지연 수행
        pass.execFunc =
            [data, execute = std::forward<ExecuteFunc>(execute)] (VkCommandBuffer commandBuffer)
            {
                execute(commandBuffer, data);
            };

        return data;
    }
}
