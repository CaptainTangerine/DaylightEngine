#pragma once

#include "VulkanResourceState.h"
#include <vma/vk_mem_alloc.h>

namespace Dlight
{
	class VulkanDevice;

	struct FVulkanBufferDesc
	{
		VkDeviceSize size = 0;
		VkBufferUsageFlags usage = 0;
		VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
		VmaAllocationCreateFlags allocationFlags = 0;
	};

	// DeviceLocal Buffer 생성
	class VulkanBuffer
	{
	public:
		VulkanBuffer(VulkanDevice& _device, const FVulkanBufferDesc& desc);
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;
		VulkanBuffer(VulkanBuffer&&) = delete;
		VulkanBuffer& operator=(VulkanBuffer&&) = delete;

	public:
		VkBuffer GetBuffer() const { return buffer; };
		VkDeviceSize GetSize() const { return size; };
		VkBufferUsageFlags GetUsage() const { return usage; };
		void* GetMappedData() const { return mappedData; };
		void FlushAllocation(VkDeviceSize offset, VkDeviceSize flushSize);

	private:
		VulkanDevice& device;

		VkBuffer buffer = { VK_NULL_HANDLE };
		VmaAllocation allocation = { VK_NULL_HANDLE };
		VkDeviceSize size = { 0 };
		VkBufferUsageFlags usage = { 0 };

		void* mappedData = { nullptr };

	};

}
