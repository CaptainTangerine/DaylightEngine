#pragma once
#include "vulkan/vulkan.h"
#include "RenderGraphHandle.h"
#include "Utils/ResourceState.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include "Graphics/Vulkan/VulkanBuffer.h"

namespace Dlight
{
    // 렌더 그래프는 패스간의 의존성을 관리한다.
    // 복잡하게 하지 않고 노드를 Resource와 Pass로 나누지 않고 Pass로 둠
    // Memory Aliasing은 추후 과제로 남겨두고, 각 Resource의 의존성 관리 Pass 추가의 확장성, 유지보수를 목적으로 한다.
    // 렌더 그래프의 리소스들은 대부분 이미지이긴 하지만 확장 가능성을 열어둠

    class RenderGraphPass;
    class VulkanDevice;

    struct FRenderGraphTextureResource
    {
        FVulkanImageDesc description;
        std::unique_ptr<VulkanImage> physicalResource;
    };

    struct FRenderGraphBufferResource
    {
        FVulkanBufferDesc description;
        std::unique_ptr<VulkanBuffer> physicalResource;
    };

    using RenderGraphResource = std::variant<FRenderGraphTextureResource, FRenderGraphBufferResource>;

    struct PassNode
    {
        std::string name;
        std::map<RenderGraphResourceID, RenderGraphResourceState> reads;
        std::map<RenderGraphResourceID, RenderGraphResourceState> writes;
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
            RenderGraphImageHandle  Declare(FVulkanImageDesc desc);
            RenderGraphBufferHandle Declare(FVulkanBufferDesc desc);


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
        void Execute(VkCommandBuffer commandBuffer);
    private:
        PassNode& CreateRenderPass(const std::string& passName);
        RenderGraphImageHandle DeclareImageResource(FVulkanImageDesc desc);
        //DeclareBufferResource();

    private:
        VulkanDevice& device;

        std::vector<std::unique_ptr<PassNode>> passes;
        std::vector<RenderGraphResource> resources;

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
