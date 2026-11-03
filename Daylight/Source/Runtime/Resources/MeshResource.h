#pragma once
#include <vulkan/vulkan.h>

#include "Resource.h"

namespace Dlight
{
    class VulkanDevice;
    class VulkanBuffer;
    class TextureResource;


    struct FVertexLayout
    {
        uint32 stride = 0;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };

    struct FMeshRaw
    {
        std::vector<std::byte> vertexData;
        FVertexLayout layout;
        std::vector<uint32> indices;
    };

    class MeshResource : public Resource
    {
    public:
        MeshResource(const std::string& id, VulkanDevice& _device, FMeshRaw _meshData);
        ~MeshResource() override;

        const FVertexLayout& GetVertexLayout() const { return meshData.layout; }
        const VulkanBuffer* GetVertexBuffer() const { return vertexBuffer.get(); }
        const VulkanBuffer* GetIndexBuffer() const { return indexBuffer.get(); }
        uint32 GetVertexCount() const { return vertexCount; }
        uint32 GetIndexCount() const { return indexCount; }
        VkIndexType GetIndexType() const { return VK_INDEX_TYPE_UINT32; }

    protected:
        bool DoLoad() override;
        void DoUnload() override;

    private:
        std::unique_ptr<VulkanBuffer> CreateVertexBuffer(const void* data, VkDeviceSize byteSize);
        std::unique_ptr<VulkanBuffer> CreateIndexBuffer(const void* data, VkDeviceSize byteSize);

    private:
        VulkanDevice& device;
        FMeshRaw meshData;

        std::unique_ptr<VulkanBuffer> vertexBuffer;
        std::unique_ptr<VulkanBuffer> indexBuffer;
        uint32 vertexCount = 0;
        uint32 indexCount = 0;

        ResourceHandle<TextureResource> TextureResource;
    };
}
