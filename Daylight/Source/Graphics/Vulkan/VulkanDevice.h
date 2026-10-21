#pragma once

#include <vulkan/vulkan.h>
#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "VulkanResourceState.h"
#include "VulkanSwapchain.h"

struct SDL_Window;

namespace Dlight
{
	class VulkanSwapchain;
	class VulkanBuffer;
	class VulkanImage;
	class Image;

	struct FFrameResource
	{
		uint32 frameSlot = { 0 };
		VkCommandPool commandPool = { nullptr };
		VkCommandBuffer commandBuffer = { nullptr };
		VkSemaphore imageAcquiredSemaphore = { nullptr };
	};

	class VulkanDevice
	{
	public:
		constexpr static uint32   vulkanVersion = { VK_API_VERSION_1_4 };
		constexpr static uint32   MaxFramesInFlight = { 2 };

	public:
		VulkanDevice(SDL_Window* window, uint32 width, uint32 height);
		~VulkanDevice();

	public:
		VkInstance GetInstance() const { return vulkanInstance; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkDevice GetDevice() const { return device; }
		VkQueue GetGraphicsQueue() const { return gfxQueue; }
		uint32 GetGraphicsAndComputeQueueFamilyIndex() const { return gfxAndComputeQueueFamilyIndex; }
		VkSurfaceKHR GetSurface() const { return vulkanSurface; }
		VmaAllocator GetVmaAllocator() const { return vmaAllocator;  }
		VulkanSwapchain& GetSwapchain() { return *swapchain; }
		const VulkanSwapchain& GetSwapchain() const { return *swapchain; }
		FFrameResource& GetCurrentFrameResource() { return frameResources[frameIndex % MaxFramesInFlight]; }
		const FFrameResource& GetCurrentFrameResource() const { return frameResources[frameIndex % MaxFramesInFlight]; }
		const VkImage GetCurrentSwapchainImage() const { return swapchain->GetSwapchianImage(imageIndex); };

		VkDescriptorSetLayout GetBindlessLayout() const { return bindlessLayout; }
		VkDescriptorSet GetBindlessSet() const { return bindlessSet; }
		VkDescriptorSetLayout GetSamplerLayout() const { return samplerLayout; }
		VkDescriptorSet GetSamplerSet() const { return samplerSet; }

	public:
		// 현재 사용가능한 이미지인덱스를 PresentEngine에서 가져온다.
		bool AcquireNextImage();
		// false이면 재생성이 보류되었으므로 이번 프레임을 건너뛴다.
		bool UpdateSwapchain(uint32 width, uint32 height, bool& recreated);

		void BeginFrame();
		void EndFrame();

	public:
		std::unique_ptr<VulkanBuffer> CreateVertexBuffer(const void* data, VkDeviceSize byteSize);
		std::unique_ptr<VulkanBuffer> CreateIndexBuffer(const void* data, VkDeviceSize byteSize);
		std::unique_ptr<VulkanImage> CreateImage2D(Image& image);

	private:
		// CPU Data -> DeviceLocal
		void UploadImage(VulkanImage& destination, const void* data, VkDeviceSize byteSize, EResourceState beforeState, EResourceState finalState);
		void TransitionImage(VkCommandBuffer commandBuufer, const VulkanImage& image, EResourceState _src, EResourceState _dst);
		void UploadBuffer(VulkanBuffer& destination, const void* data, VkDeviceSize size, VkDeviceSize destinationOffset, EResourceState beforeState, EResourceState finalState);
		void TransitionBuffer(VkCommandBuffer commandBuffer, const VulkanBuffer& buffer, EResourceState _src, EResourceState _dst);

	private:
		void Initialize(SDL_Window* window, uint32 width, uint32 height);
		void Shutdown();

		bool InitializeVulkan();
		bool InitializeSurface(SDL_Window* window);
		VkPhysicalDevice FindPhysicalDevice() const;
		bool FindGraphicsQueue();
		bool CreateDevice();
		bool InitializeVMA();
		bool CreateSyncResources();
		bool CreateCommandBuffers();

		bool CreateBindlessDescriptors();
		bool CreateSamplerDescriptors();
		void DestroyDescriptors();


	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		bool volkInitialized = { false };

		// Vulkan Core
		VkInstance vulkanInstance = { VK_NULL_HANDLE };
		VkDebugUtilsMessengerEXT debugMessenger = { VK_NULL_HANDLE };
		VkSurfaceKHR vulkanSurface = { VK_NULL_HANDLE };
		VkPhysicalDevice physicalDevice = { VK_NULL_HANDLE };
		VkDevice device = { VK_NULL_HANDLE };

		// Queue Related
		uint32 gfxAndComputeQueueFamilyIndex = { UINT32_MAX } ;
		VkQueue gfxQueue = { VK_NULL_HANDLE };

		// Vulkan Memory Allocater
		VmaAllocator vmaAllocator = { nullptr };

		// Swapchain
		std::unique_ptr<VulkanSwapchain> swapchain;
		bool bRequireRecreateSwapchain = { false };
		uint32 imageIndex = { 0 };

		// Frame and synchroniztion resources
		VkSemaphore timelineSemaphore = { nullptr };
		std::array<FFrameResource, MaxFramesInFlight> frameResources;
		uint64 frameIndex = { 0 };
		uint64 nextSignalValue = { MaxFramesInFlight + 1 };

		// Bindless DesciptorSet
		constexpr static uint32 MaxTextures = { 64 };
		VkDescriptorPool bindlessPool = { VK_NULL_HANDLE };
		VkDescriptorSetLayout bindlessLayout = { VK_NULL_HANDLE };
		VkDescriptorSet bindlessSet = { VK_NULL_HANDLE };

		// Sampler DescriptorSet
		VkDescriptorPool samplerPool = { VK_NULL_HANDLE };
		VkDescriptorSetLayout samplerLayout = { VK_NULL_HANDLE };
		VkDescriptorSet samplerSet = { VK_NULL_HANDLE };

	};
}
