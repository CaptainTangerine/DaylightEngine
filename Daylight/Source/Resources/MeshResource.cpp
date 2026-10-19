#include "pch.h"
#include "MeshResource.h"

#include <limits>

#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanDevice.h"

namespace Dlight
{
    MeshResource::MeshResource(const std::string& id, VulkanDevice& _device, FMeshRaw _meshData)
        : Resource(id)
        , device(_device)
        , meshData(std::move(_meshData))
    {}

    MeshResource::~MeshResource() = default;

    bool MeshResource::DoLoad()
    {
        if (IsLoaded())
        {
            return true;
        }

        const uint32 stride = meshData.layout.stride;

        if (meshData.vertexData.empty() || meshData.indices.empty() ||
            stride == 0 || meshData.vertexData.size() % stride != 0 ||
            meshData.layout.attributes.empty())
        {
            DL_LOG_ERROR("Invalid mesh data: ", GetID());
            return false;
        }

        const size_t rawVertexCount = meshData.vertexData.size() / stride;

        if (rawVertexCount > std::numeric_limits<uint32>::max() ||
            meshData.indices.size() > std::numeric_limits<uint32>::max())
        {
            DL_LOG_ERROR("Mesh element count exceeds uint32 range: ", GetID());
            return false;
        }

        vertexBuffer = CreateVertexBuffer(
            meshData.vertexData.data(), meshData.vertexData.size());
        indexBuffer = CreateIndexBuffer(
            meshData.indices.data(), meshData.indices.size() * sizeof(uint32));

        vertexCount = static_cast<uint32>(rawVertexCount);
        indexCount = static_cast<uint32>(meshData.indices.size());
        return true;
    }

    std::unique_ptr<VulkanBuffer> MeshResource::CreateVertexBuffer(const void* data, VkDeviceSize byteSize)
    {
        FVulkanBufferDesc desc = {};
        desc.size = byteSize;
        desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        desc.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        auto buffer = std::make_unique<VulkanBuffer>(device, desc);

        device.UploadBuffer(*buffer, data, byteSize, 0, EResourceState::Undefined, EResourceState::VertexBuffer);

        return buffer;
    }

    std::unique_ptr<VulkanBuffer> MeshResource::CreateIndexBuffer(const void* data, VkDeviceSize byteSize)
    {
        FVulkanBufferDesc desc = {};
        desc.size = byteSize;
        desc.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        desc.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        auto buffer = std::make_unique<VulkanBuffer>(device, desc);

        device.UploadBuffer(*buffer, data, byteSize, 0, EResourceState::Undefined, EResourceState::IndexBuffer);

        return buffer;
    }

    void MeshResource::DoUnload()
    {
        vertexBuffer.reset();
        indexBuffer.reset();
        vertexCount = 0;
        indexCount = 0;
    }

}
