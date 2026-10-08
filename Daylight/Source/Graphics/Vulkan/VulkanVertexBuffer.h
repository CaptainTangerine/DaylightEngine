#pragma once
#include "VulkanDevice.h"

namespace Dlight
{
    class VulkanVertexBuffer
    {
    public:
        VulkanVertexBuffer(VulkanDevice& _device, const void* vertexData, VkDeviceSize byteSize);
        ~VulkanVertexBuffer();

        VulkanVertexBuffer(const VulkanVertexBuffer&) = delete;
        VulkanVertexBuffer& operator=(const VulkanVertexBuffer&) = delete;

        VkBuffer GetBuffer() const { return buffer; }

    private:
        void Initialize(const void* vertexData, VkDeviceSize byteSize);
        
        //스테이징 버퍼에 정점 데이터를 GPU DeviceLocal에 있는 곳으로 복사
        void CopyFromStaging(VkBuffer stagingBuffer, VkDeviceSize byteSize);

    private:
        VulkanDevice& device;

        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
    };
}
