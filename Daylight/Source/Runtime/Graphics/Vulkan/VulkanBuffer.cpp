#include "pch.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"

namespace Dlight
{
    VulkanBuffer::VulkanBuffer(VulkanDevice& _device, const FVulkanBufferDesc& desc)
        : device(_device), size(desc.size), usage(desc.usage)
    {
        if (size == 0 || usage == 0)
        {
            DL_LOG_ERROR("Invalid VulkanBuffer description");
            std::abort();
        }

        const VmaAllocator allocator = device.GetVmaAllocator();

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = desc.size;
        bufferInfo.usage = desc.usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = desc.memoryUsage;
        allocInfo.flags = desc.allocationFlags;

        VmaAllocationInfo resultInfo{};

        if (VK_SUCCESS != vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &resultInfo))
        {
            DL_LOG_ERROR("Can't create VulkanBuffer");
            std::abort();
        }
        mappedData = resultInfo.pMappedData;
    }
    
    VulkanBuffer::~VulkanBuffer()
    {
        if (VK_NULL_HANDLE != buffer)
        {
            vmaDestroyBuffer(device.GetVmaAllocator(), buffer, allocation);
        }
    }
    void VulkanBuffer::FlushAllocation(VkDeviceSize offset, VkDeviceSize flushSize)
    {
        if (offset + flushSize > size)
        {
            DL_LOG_ERROR("VulkanBuffer flush range is out of bounds");
            std::abort();
        }

        if (VK_SUCCESS != vmaFlushAllocation(device.GetVmaAllocator(), allocation, offset, flushSize))
        {
            DL_LOG_ERROR("Failed to flush VulkanBuffer");
            std::abort();
        }
    }
}
