#include "pch.h"
#include "RenderGraph.h"
#include "Graphics/Vulkan/VulkanImage.h"
#include <cassert>


namespace Dlight
{
    RenderGraph::Builder::Builder(RenderGraph& rg, PassNode& _passNode) noexcept
        : renderGraph(rg), passNode(_passNode)
    {
    }

    RenderGraphImageHandle RenderGraph::Builder::Read(RenderGraphImageHandle resource, EImageState usage)
    {
        if (!resource.IsInitialized())
        {
            assert(false && "Read requires an initialized image handle");
            return resource;
        }

        const bool inserted = passNode.reads.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

        if (!inserted)
        {
            assert(false && "Duplicate Read in the same pass");
            resource.Clear();
        }

        return resource;
    }

    RenderGraphBufferHandle RenderGraph::Builder::Read(RenderGraphBufferHandle resource, EBufferState usage)
    {
        if (!resource.IsInitialized())
        {
            assert(false && "Read requires an initialized buffer handle");
            return resource;
        }

        const bool inserted = passNode.reads.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

        if (!inserted)
        {
            assert(false && "Duplicate Read in the same pass");
            resource.Clear();
        }

        return resource;
    }

    RenderGraphImageHandle RenderGraph::Builder::Write(RenderGraphImageHandle resource, EImageState usage)
    {
        if (!resource.IsInitialized())
        {
            assert(false && "Write requires an initialized image handle");
            return resource;
        }

        const bool inserted = passNode.writes.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

        if (!inserted)
        {
            assert(false && "Duplicate Write in the same pass");
            resource.Clear();
        }

        return resource;
    }

    RenderGraphBufferHandle RenderGraph::Builder::Write(RenderGraphBufferHandle resource, EBufferState usage)
    {
        if (!resource.IsInitialized())
        {
            assert(false && "Write requires an initialized buffer handle");
            return resource;
        }

        const bool inserted = passNode.writes.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

        if (!inserted)
        {
            assert(false && "Duplicate Write in the same pass");
            resource.Clear();
        }

        return resource;
    }

    RenderGraphImageHandle RenderGraph::Builder::Declare(FVulkanImageDesc desc)
    {
        return renderGraph.DeclareImageResource(desc);
    }

    RenderGraphBufferHandle RenderGraph::Builder::Declare(FVulkanBufferDesc desc)
    {
        // TODO
        //renderGraph.BufferResource()
    }


    RenderGraph::RenderGraph(VulkanDevice& device) noexcept
        : device(device)
    {
    }

    RenderGraph::~RenderGraph() noexcept
    {
    }

    void RenderGraph::Execute(VkCommandBuffer commandBuffer)
    {
        for (const std::unique_ptr<PassNode>& pass : passes)
        {
            assert(pass->execFunc && "Pass has no execute function");
            pass->execFunc(commandBuffer);
        }
    }

    PassNode& RenderGraph::CreateRenderPass(const std::string& passName)
    {
        std::unique_ptr<PassNode> pass = std::make_unique<PassNode>();
        pass->name = passName;
        passes.push_back(std::move(pass));

        return *passes.back();
    }

    RenderGraphImageHandle RenderGraph::DeclareImageResource(FVulkanImageDesc desc)
    {
        const uint32 index = static_cast<uint32>(resources.size());
        resources.emplace_back(FRenderGraphTextureResource{desc, nullptr});
        
        return  RenderGraphImageHandle{ 
            RenderGraphResourceID{ERenderGraphResourceType::Image, index}
        };

    }
}
