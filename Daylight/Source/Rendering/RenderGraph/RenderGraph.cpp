#include "pch.h"
#include "RenderGraph.h"
#include "PassNode.h"
#include "ResourceNode.h"

namespace Dlight
{
    RenderGraph::Builder::Builder(RenderGraph& rg, PassNode* _passNode) noexcept
        : renderGraph(rg), passNode(_passNode)
    {
    }
}
