#pragma once

namespace Dlight
{
    // 렌더 그래프는 패스간의 의존성을 관리한다.
    // Memory Aliasing은 추후 과제로 남겨두고, 각 Resource의 의존성 관리 Pass 추가의 확장성, 유지보수를 목적으로 한다.
    
    
    // depGraph node
    class PassNode;
    class ResourceNode;

    class RenderGraphPass;
    class RenderGraphResource;

    class RenderGraph
    {
    public:
        class PassNode;
        class ResourceNode;

        class Builder
        {
            Builder(Builder const&) = delete;
            Builder& operator=(Builder const&) = delete;

            // DeclareRenderPass
            // Read
            // Write

        private:
            friend class RenderGraph;
            Builder(RenderGraph& rg, PassNode* _passNode) noexcept;
            ~Builder() noexcept = default;

        private:
            RenderGraph& renderGraph;
            PassNode* passNode;
        };

        RenderGraph(RenderGraph const&) = delete;
        RenderGraph& operator=(RenderGraph const&) = delete;
        ~RenderGraph() noexcept;

    public:
        // RenderGraph API
        template<typename Data, typename Setup, typename Execute>
        RenderGraphPass& AddPass(const std::string& passName, Setup&& setup, Execute&& execute);
        // Compile
        // Execute


    private:
        std::vector<ResourceNode*> resources;
        std::vector<PassNode*> passes;
    };

    template<typename Data, typename Setup, typename Execute>
    RenderGraphPass& RenderGraph::AddPass(const std::string& passName, Setup&& setup, Execute&& execute)
    {
        //SetUp

        //Execute

    }
}
