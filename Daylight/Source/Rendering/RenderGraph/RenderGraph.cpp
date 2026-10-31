#include "pch.h"
#include "RenderGraph.h"
#include "Graphics/Vulkan/VulkanDevice.h"
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
			return resource;
		}

		const bool inserted = passNode.reads.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

		if (!inserted)
		{
			resource.Clear();
		}

		return resource;
	}

	RenderGraphBufferHandle RenderGraph::Builder::Read(RenderGraphBufferHandle resource, EBufferState usage)
	{
		if (!resource.IsInitialized())
		{
			return resource;
		}

		const bool inserted = passNode.reads.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

		if (!inserted)
		{
			resource.Clear();
		}

		return resource;
	}

	RenderGraphImageHandle RenderGraph::Builder::Write(RenderGraphImageHandle resource, EImageState usage)
	{
		if (!resource.IsInitialized())
		{
			return resource;
		}

		const bool inserted = passNode.writes.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

		if (!inserted)
		{
			resource.Clear();
		}

		return resource;
	}

	RenderGraphBufferHandle RenderGraph::Builder::Write(RenderGraphBufferHandle resource, EBufferState usage)
	{
		if (!resource.IsInitialized())
		{
			return resource;
		}

		const bool inserted = passNode.writes.emplace(resource.GetID(), RenderGraphResourceState{ usage }).second;

		if (!inserted)
		{
			resource.Clear();
		}

		return resource;
	}

	RenderGraphImageHandle RenderGraph::Builder::DeclareInternal(FVulkanImageDesc desc)
	{
		return renderGraph.DeclareInternalResource(desc);
	}

	RenderGraphBufferHandle RenderGraph::Builder::DeclareInternal(FVulkanBufferDesc desc)
	{
		// TODO
		//renderGraph.BufferResource()
		return renderGraph.DeclareInternalResource(desc);
	}

	RenderGraphImageHandle RenderGraph::Builder::DeclareExternal(FVulkanImageDesc desc, EImageState initialState, std::optional<EImageState> finalState)
	{
		return renderGraph.DeclareExternalResource(desc, initialState, finalState);
	}

	RenderGraphBufferHandle RenderGraph::Builder::DeclareExternal(FVulkanBufferDesc desc, EBufferState initialState, std::optional<EBufferState> finalState)
	{
		return renderGraph.DeclareExternalResource(desc, initialState, finalState);
	}


	RenderGraph::RenderGraph(VulkanDevice& device) noexcept
		: device(device)
	{
	}

	RenderGraph::~RenderGraph() noexcept
	{
	}

	void RenderGraph::Compile()
	{
		// TODO : Pass Culling, Dependency Graph를 통해 위상정렬

		for (const auto& pass : passes)
		{
			for (const auto& [id, state] : pass->reads)
			{
				AccumulateUsage(id, state);
			}

			for (const auto& [id, state] : pass->writes)
			{
				AccumulateUsage(id, state);
			}
		}

		// 물리 리소스 생성
		for (RenderGraphResource& resource : resources)
		{
			if (auto* image = std::get_if<RenderGraphTextureResource>(&resource))
			{
				if (auto* internal = std::get_if<FInternalRenderGraphTexture>(image))
				{
					CreatePhysicalResource(*internal);
				}
			}
			else if (auto* buffer = std::get_if<RenderGraphBufferResource>(&resource))
			{
				if (auto* internal = std::get_if<FInternalRenderGraphBuffer>(buffer))
				{
					CreatePhysicalResource(*internal);
				}
			}
		}

		// 배리어 생성
		CreateResourceBarrier();
	}

	void RenderGraph::BindExternal(RenderGraphImageHandle handle, VkImage externalResource)
	{
		const RenderGraphResourceID& id = handle.GetID();
		RenderGraphTextureResource& texture = std::get<RenderGraphTextureResource>(resources[id.index]);
		FExternalRenderGraphTexture& resource = std::get<FExternalRenderGraphTexture>(texture);
		resource.physicalResource = externalResource;
	}

	void RenderGraph::BindExternal(RenderGraphBufferHandle handle, VkBuffer externalResource)
	{
		const RenderGraphResourceID& id = handle.GetID();
		RenderGraphBufferResource& buffer = std::get<RenderGraphBufferResource>(resources[id.index]);
		FExternalRenderGraphBuffer& resource = std::get<FExternalRenderGraphBuffer>(buffer);
		resource.physicalResource = externalResource;
	}

	void RenderGraph::Execute(VkCommandBuffer commandBuffer)
	{
		if (hasExecuted)
		{
			RecordBarriers(commandBuffer, frameStartBarriers);
		}
		else
		{
			RecordBarriers(commandBuffer, initialBarriers);
		}

		for (const std::unique_ptr<PassNode>& pass : passes)
		{
			if (!pass->execFunc)
			{
				continue;
			}

			RecordBarriers(commandBuffer, pass->barriers);
			pass->execFunc(commandBuffer);
		}

		RecordBarriers(commandBuffer, finalBarriers);
		hasExecuted = true;
	}

	PassNode& RenderGraph::CreateRenderPass(const std::string& passName)
	{
		std::unique_ptr<PassNode> pass = std::make_unique<PassNode>();
		pass->name = passName;
		passes.push_back(std::move(pass));

		return *passes.back();
	}

	RenderGraphImageHandle RenderGraph::DeclareInternalResource(FVulkanImageDesc desc)
	{
		const uint32 index = static_cast<uint32>(resources.size());
		FInternalRenderGraphTexture resource = {};
		resource.description = desc;

		resources.emplace_back(RenderGraphTextureResource{ std::move(resource) });

		return  RenderGraphImageHandle{
			RenderGraphResourceID{ERenderGraphResourceType::Image, index}
		};

	}
	RenderGraphBufferHandle RenderGraph::DeclareInternalResource(FVulkanBufferDesc desc)
	{
		const uint32 index = static_cast<uint32>(resources.size());
		FInternalRenderGraphBuffer resource = {};
		resource.description = desc;

		resources.emplace_back(RenderGraphBufferResource{ std::move(resource) });

		return RenderGraphBufferHandle{
			RenderGraphResourceID{ ERenderGraphResourceType::Buffer, index }
		};
	}
	RenderGraphImageHandle RenderGraph::DeclareExternalResource(FVulkanImageDesc desc, EImageState initialState, std::optional<EImageState> finalState)
	{
		const uint32 index = static_cast<uint32>(resources.size());
		FExternalRenderGraphTexture resource = {};
		resource.description = desc;
		resource.initialState = initialState;
		resource.finalState = finalState;

		resources.emplace_back(RenderGraphTextureResource{ std::move(resource) });

		return RenderGraphImageHandle{
			RenderGraphResourceID{ERenderGraphResourceType::Image, index}
		};
	}
	RenderGraphBufferHandle RenderGraph::DeclareExternalResource(FVulkanBufferDesc desc, EBufferState initialState, std::optional<EBufferState> finalState)
	{
		const uint32 index = static_cast<uint32>(resources.size());
		FExternalRenderGraphBuffer resource = {};
		resource.description = desc;
		resource.initialState = initialState;
		resource.finalState = finalState;

		resources.emplace_back(RenderGraphBufferResource{ std::move(resource) });

		return RenderGraphBufferHandle{
			RenderGraphResourceID{ ERenderGraphResourceType::Buffer, index }
		};
	}

	void RenderGraph::AccumulateUsage(const RenderGraphResourceID& id, const RenderGraphResourceState& state)
	{
		if (id.index >= resources.size())
		{
			return;
		}

		RenderGraphResource& resource = resources[id.index];

		if (id.type == ERenderGraphResourceType::Image)
		{
			RenderGraphTextureResource* texture = std::get_if<RenderGraphTextureResource>(&resource);
			const EImageState* imageState = std::get_if<EImageState>(&state);

			if (!texture || !imageState)
			{
				return;
			}

			GetTextureDescription(*texture).usage |= ToImageUsage(*imageState);
			return;
		}

		RenderGraphBufferResource* buffer = std::get_if<RenderGraphBufferResource>(&resource);
		const EBufferState* bufferState = std::get_if<EBufferState>(&state);

		if (!buffer || !bufferState)
		{
			return;
		}

		GetBufferDescription(*buffer).usage |= ToBufferUsage(*bufferState);
	}

	FVulkanImageDesc& RenderGraph::GetTextureDescription(RenderGraphTextureResource& resource)
	{
		if (auto* internal = std::get_if<FInternalRenderGraphTexture>(&resource))
		{
			return internal->description;
		}

		return std::get<FExternalRenderGraphTexture>(resource).description;
	}

	const FVulkanImageDesc& RenderGraph::GetTextureDescription(const RenderGraphTextureResource& resource) const
	{
		if (const auto* internal = std::get_if<FInternalRenderGraphTexture>(&resource))
		{
			return internal->description;
		}

		return std::get<FExternalRenderGraphTexture>(resource).description;
	}

	VkImage RenderGraph::GetPhysicalImage(const RenderGraphTextureResource& resource) const
	{
		if (const auto* internal = std::get_if<FInternalRenderGraphTexture>(&resource))
		{
			return internal->physicalResource
				? internal->physicalResource->GetImage()
				: VK_NULL_HANDLE;
		}

		return std::get<FExternalRenderGraphTexture>(resource).physicalResource;
	}

	void RenderGraph::CreatePhysicalResource(FInternalRenderGraphTexture& resource)
	{
		if (resource.physicalResource)
		{
			return;
		}

		resource.physicalResource = std::make_unique<VulkanImage>(device, resource.description);
		hasExecuted = false;
	}

	FVulkanBufferDesc& RenderGraph::GetBufferDescription(RenderGraphBufferResource& resource)
	{
		if (auto* internal = std::get_if<FInternalRenderGraphBuffer>(&resource))
		{
			return internal->description;
		}

		return std::get<FExternalRenderGraphBuffer>(resource).description;
	}

	const FVulkanBufferDesc& RenderGraph::GetBufferDescription(const RenderGraphBufferResource& resource) const
	{
		if (const auto* internal = std::get_if<FInternalRenderGraphBuffer>(&resource))
		{
			return internal->description;
		}

		return std::get<FExternalRenderGraphBuffer>(resource).description;
	}

	VkBuffer RenderGraph::GetPhysicalBuffer(const RenderGraphBufferResource& resource) const
	{
		if (const auto* internal = std::get_if<FInternalRenderGraphBuffer>(&resource))
		{
			return internal->physicalResource
				? internal->physicalResource->GetBuffer()
				: VK_NULL_HANDLE;
		}

		return std::get<FExternalRenderGraphBuffer>(resource).physicalResource;
	}

	void RenderGraph::CreatePhysicalResource(FInternalRenderGraphBuffer& resource)
	{
		if (resource.physicalResource)
		{
			return;
		}

		resource.physicalResource = std::make_unique<VulkanBuffer>(device, resource.description);
		hasExecuted = false;
	}

	bool RenderGraph::NeedsBarrier(const RenderGraphResourceID& id, const FTrackedAccess& previous, const FTrackedAccess& current)
	{
		if (!previous.hasPreviousAccess)
		{
			return id.type == ERenderGraphResourceType::Image;
		}

		if (id.type == ERenderGraphResourceType::Image &&
			previous.layout != current.layout)
		{
			return true;
		}

		return previous.isWrite || current.isWrite;
	}

	FRenderGraphBarrierPlan RenderGraph::MakeBarrierPlan(const RenderGraphResourceID& id, const FTrackedAccess& previous, const FTrackedAccess& current)
	{
		if (id.type == ERenderGraphResourceType::Image)
		{
			FImageBarrierPlan plan = {};
			plan.srcStage = previous.stage;
			plan.srcAccess = previous.access;
			plan.oldLayout = previous.layout;
			plan.dstStage = current.stage;
			plan.dstAccess = current.access;
			plan.newLayout = current.layout;
			return plan;
		}

		FBufferBarrierPlan plan = {};
		plan.srcStage = previous.stage;
		plan.srcAccess = previous.access;
		plan.dstStage = current.stage;
		plan.dstAccess = current.access;
		return plan;
	}

	void RenderGraph::InitializeExternalAccesses(std::vector<FTrackedAccess>& lastAccesses)
	{
		for (uint32 index = 0; index < resources.size(); ++index)
		{
			if (auto* texture = std::get_if<RenderGraphTextureResource>(&resources[index]))
			{
				if (auto* external = std::get_if<FExternalRenderGraphTexture>(texture))
				{
					lastAccesses[index] = ConvertToTrackedAccess(
						RenderGraphResourceState{ external->initialState },
						IsWriteState(external->initialState));
				}
			}
			else if (auto* buffer = std::get_if<RenderGraphBufferResource>(&resources[index]))
			{
				if (auto* external = std::get_if<FExternalRenderGraphBuffer>(buffer))
				{
					lastAccesses[index] = ConvertToTrackedAccess(
						RenderGraphResourceState{ external->initialState },
						IsWriteState(external->initialState));
				}
			}
		}
	}

	void RenderGraph::CreateExternalFinalBarriers(const std::vector<FTrackedAccess>& lastAccesses)
	{
		finalBarriers.clear();

		for (uint32 index = 0; index < resources.size(); ++index)
		{
			if (auto* texture = std::get_if<RenderGraphTextureResource>(&resources[index]))
			{
				if (auto* external = std::get_if<FExternalRenderGraphTexture>(texture))
				{
					if (external->finalState)
					{
						const RenderGraphResourceID id{ ERenderGraphResourceType::Image, index };
						const EImageState state = *external->finalState;
						const FTrackedAccess finalAccess = ConvertToTrackedAccess(RenderGraphResourceState{ state }, IsWriteState(state));

						if (NeedsBarrier(id, lastAccesses[index], finalAccess))
						{
							finalBarriers.emplace(id, MakeBarrierPlan(id, lastAccesses[index], finalAccess));
						}
					}
				}
			}
			else if (auto* buffer = std::get_if<RenderGraphBufferResource>(&resources[index]))
			{
				if (auto* external = std::get_if<FExternalRenderGraphBuffer>(buffer))
				{
					if (external->finalState)
					{
						const RenderGraphResourceID id{ ERenderGraphResourceType::Buffer, index };
						const EBufferState state = *external->finalState;
						const FTrackedAccess finalAccess = ConvertToTrackedAccess(RenderGraphResourceState{ state }, IsWriteState(state));

						if (NeedsBarrier(id, lastAccesses[index], finalAccess))
						{
							finalBarriers.emplace(id, MakeBarrierPlan(id, lastAccesses[index], finalAccess));
						}
					}
				}
			}
		}
	}


	void RenderGraph::CreateResourceBarrier()
	{
		std::vector<FTrackedAccess> firstAccesses(resources.size());
		std::vector<FTrackedAccess> lastAccesses(resources.size());
		std::vector<bool> internalResources(resources.size());
		std::vector<bool> firstReadGroupOpen(resources.size(), true);

		InitializeExternalAccesses(lastAccesses);
		initialBarriers.clear();
		frameStartBarriers.clear();

		for (uint32 index = 0; index < resources.size(); ++index)
		{
			if (auto* texture = std::get_if<RenderGraphTextureResource>(&resources[index]))
			{
				internalResources[index] = std::holds_alternative<FInternalRenderGraphTexture>(*texture);
			}
			else if (auto* buffer = std::get_if<RenderGraphBufferResource>(&resources[index]))
			{
				internalResources[index] = std::holds_alternative<FInternalRenderGraphBuffer>(*buffer);
			}
		}

		for (const auto& pass : passes)
		{
			pass->barriers.clear();

			// 일단 한 패스에서 한 리소스에 대한 R/W가 동시에 되면 에러 처리
			for (const auto& [id, state] : pass->reads)
			{
				assert(pass->writes.find(id) == pass->writes.end() && "The same resource cannot be R/W in one pass");
			}

			for (const auto& [id, state] : pass->reads)
			{
				FTrackedAccess& src = lastAccesses[id.index];
				FTrackedAccess dst = ConvertToTrackedAccess(state, false);
				FTrackedAccess& first = firstAccesses[id.index];
				const bool isFirstAccess = !first.hasPreviousAccess;

				if (isFirstAccess)
				{
					first = dst;
				}
				else if (firstReadGroupOpen[id.index] && first.layout == dst.layout)
				{
					first.stage |= dst.stage;
					first.access |= dst.access;
				}
				else
				{
					firstReadGroupOpen[id.index] = false;
				}

				if (!(internalResources[id.index] && isFirstAccess) && NeedsBarrier(id, src, dst))
				{
					pass->barriers.emplace(id, MakeBarrierPlan(id, src, dst));
				}

				// R → R이며 Layout이 같으면 누적
				if (src.hasPreviousAccess &&
					!src.isWrite &&
					src.layout == dst.layout)
				{
					src.stage |= dst.stage;
					src.access |= dst.access;
					continue;
				}

				src = dst;
			}

			for (const auto& [id, state] : pass->writes)
			{
				FTrackedAccess& src = lastAccesses[id.index];
				FTrackedAccess dst = ConvertToTrackedAccess(state, true);
				FTrackedAccess& first = firstAccesses[id.index];
				const bool isFirstAccess = !first.hasPreviousAccess;

				if (isFirstAccess)
				{
					first = dst;
				}

				firstReadGroupOpen[id.index] = false;

				if (!(internalResources[id.index] && isFirstAccess) && NeedsBarrier(id, src, dst))
				{
					pass->barriers.emplace(id, MakeBarrierPlan(id, src, dst));
				}

				src = dst;
			}
		}

		for (uint32 index = 0; index < resources.size(); ++index)
		{
			if (!internalResources[index] || !firstAccesses[index].hasPreviousAccess)
			{
				continue;
			}

			const ERenderGraphResourceType type = std::holds_alternative<RenderGraphTextureResource>(resources[index])
				? ERenderGraphResourceType::Image
				: ERenderGraphResourceType::Buffer;
			const RenderGraphResourceID id{ type, index };
			const FTrackedAccess initialAccess = {};

			if (NeedsBarrier(id, initialAccess, firstAccesses[index]))
			{
				initialBarriers.emplace(id, MakeBarrierPlan(id, initialAccess, firstAccesses[index]));
			}

			if (NeedsBarrier(id, lastAccesses[index], firstAccesses[index]))
			{
				frameStartBarriers.emplace(id, MakeBarrierPlan(id, lastAccesses[index], firstAccesses[index]));
			}
		}

		CreateExternalFinalBarriers(lastAccesses);
	}

	void RenderGraph::RecordBarriers(VkCommandBuffer commandBuffer, const std::map<RenderGraphResourceID, FRenderGraphBarrierPlan>& barriers)
	{
		for (const auto& [id, barrierPlan] : barriers)
		{
			if (const auto* imageBarrierPlan = std::get_if<FImageBarrierPlan>(&barrierPlan))
			{
				RecordBarrier(commandBuffer, id, *imageBarrierPlan);
			}
			else if (const auto* bufferBarrierPlan = std::get_if<FBufferBarrierPlan>(&barrierPlan))
			{
				RecordBarrier(commandBuffer, id, *bufferBarrierPlan);
			}
		}
	}

	void RenderGraph::RecordBarrier(VkCommandBuffer commandBuffer, RenderGraphResourceID id, const FImageBarrierPlan& barrierPlan)
	{
		if (id.type != ERenderGraphResourceType::Image || id.index >= resources.size())
		{
			return;
		}

		const RenderGraphTextureResource* resource =
			std::get_if<RenderGraphTextureResource>(&resources[id.index]);
		if (!resource)
		{
			return;
		}

		const VkImage physicalImage = GetPhysicalImage(*resource);
		if (physicalImage == VK_NULL_HANDLE)
		{
			return;
		}

		const FVulkanImageDesc& description = GetTextureDescription(*resource);

		VkImageMemoryBarrier2 imageBarrier = {};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageBarrier.srcStageMask = barrierPlan.srcStage;
		imageBarrier.srcAccessMask = barrierPlan.srcAccess;
		imageBarrier.dstStageMask = barrierPlan.dstStage;
		imageBarrier.dstAccessMask = barrierPlan.dstAccess;
		imageBarrier.oldLayout = barrierPlan.oldLayout;
		imageBarrier.newLayout = barrierPlan.newLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = physicalImage;
		imageBarrier.subresourceRange.aspectMask = description.aspectMask;
		imageBarrier.subresourceRange.baseMipLevel = 0;
		imageBarrier.subresourceRange.levelCount = description.mipLevels;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = description.arrayLayers;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}
	void RenderGraph::RecordBarrier(VkCommandBuffer commandBuffer, RenderGraphResourceID id, const FBufferBarrierPlan& barrierPlan)
	{
		if (id.type != ERenderGraphResourceType::Buffer || id.index >= resources.size())
		{
			return;
		}

		const RenderGraphBufferResource* resource =
			std::get_if<RenderGraphBufferResource>(&resources[id.index]);
		if (!resource)
		{
			return;
		}

		const VkBuffer physicalBuffer = GetPhysicalBuffer(*resource);
		if (physicalBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		const FVulkanBufferDesc& description = GetBufferDescription(*resource);

		VkBufferMemoryBarrier2 bufferBarrier = {};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
		bufferBarrier.srcStageMask = barrierPlan.srcStage;
		bufferBarrier.srcAccessMask = barrierPlan.srcAccess;
		bufferBarrier.dstStageMask = barrierPlan.dstStage;
		bufferBarrier.dstAccessMask = barrierPlan.dstAccess;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.buffer = physicalBuffer;
		bufferBarrier.offset = 0;
		bufferBarrier.size = description.size;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.bufferMemoryBarrierCount = 1;
		dependencyInfo.pBufferMemoryBarriers = &bufferBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}
}
