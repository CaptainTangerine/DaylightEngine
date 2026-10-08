#include "pch.h"
#include "VulkanVertexBuffer.h"

Dlight::VulkanVertexBuffer::VulkanVertexBuffer(VulkanDevice& _device, const void* vertexData, VkDeviceSize byteSize)
	: device(_device)
{
    if (!vertexData || byteSize == 0)
    {
        DL_LOG_ERROR("Invalid VertexData Error");
    }

    Initialize(vertexData, byteSize);
}

Dlight::VulkanVertexBuffer::~VulkanVertexBuffer()
{
    if (VK_NULL_HANDLE != buffer)
    {
        vmaDestroyBuffer(device.GetVmaAllocator(), buffer, allocation);
    }
}

void Dlight::VulkanVertexBuffer::Initialize(const void* vertexData, VkDeviceSize byteSize)
{
    // 최종 VertexBuffer 버퍼
    const VmaAllocator allocator = device.GetVmaAllocator();

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = byteSize;
    bufferInfo.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    if (VK_SUCCESS != vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr))
    {
        DL_LOG_ERROR("Can't create vertex buffer");
    }

    // 스테이징 버퍼 
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = nullptr;
    VmaAllocationInfo stagingInfo{};

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = byteSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if ((VK_SUCCESS != vmaCreateBuffer(allocator, &stagingBufferInfo , &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingInfo)) ||
         nullptr == stagingInfo.pMappedData)
    {
        DL_LOG_ERROR("Can't create vertex staging buffer");
    }

    std::memcpy(stagingInfo.pMappedData, vertexData, static_cast<size_t>(byteSize));

    vmaFlushAllocation(allocator, stagingAllocation, 0, byteSize);
    CopyFromStaging(stagingBuffer, byteSize);

    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void Dlight::VulkanVertexBuffer::CopyFromStaging(VkBuffer stagingBuffer, VkDeviceSize byteSize)
{
    // 업로드시 임시 커맨드 풀
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = device.GetGraphicsQueueFamilyIndex();

    if (VK_SUCCESS != vkCreateCommandPool(device.GetDevice(), &poolInfo, nullptr, &commandPool))
    {
        DL_LOG_ERROR("Can't create Command Pool");
    }

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    if (VK_SUCCESS != vkAllocateCommandBuffers(device.GetDevice(), &allocateInfo, &commandBuffer))
    {
        DL_LOG_ERROR("Can't Allocate Command Buffer");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // 버퍼 복사 명령
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = byteSize;

    vkCmdCopyBuffer(commandBuffer, stagingBuffer, buffer, 1, &copyRegion);

    // Staging 복사(쓰기) 단계 이후에 작업이 이뤄져야하고 
    // GPU 정점 읽는 단계전에 복사가 끝나야함
    VkBufferMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = byteSize;

    VkDependencyInfo depInfo = {};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.bufferMemoryBarrierCount = 1;
    depInfo.pBufferMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &depInfo);

    vkEndCommandBuffer(commandBuffer);

    VkCommandBufferSubmitInfo commandSubmitInfo = {};
    commandSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandSubmitInfo.commandBuffer = commandBuffer;

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandSubmitInfo;

    vkQueueSubmit2(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.GetGraphicsQueue());

    vkDestroyCommandPool(device.GetDevice(), commandPool, nullptr);
}
