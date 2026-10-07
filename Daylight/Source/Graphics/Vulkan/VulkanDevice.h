#pragma once

#include <vulkan/vulkan.h>
#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

struct SDL_Window;

namespace Dlight
{
	class VulkanSwapchain;

	struct FrameResource
	{
		VkCommandPool commandPool = { nullptr };
		VkCommandBuffer commandBuffer = { nullptr };
		VkSemaphore imageAcquiredSemaphore = { nullptr };
	};

	struct RenderFrameContext
	{
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		// BeginFrame transitions these to COLOR_ATTACHMENT_OPTIMAL and
		// DEPTH_STENCIL_ATTACHMENT_OPTIMAL respectively; clear before use.
		VkImageView colorImageView = VK_NULL_HANDLE;
		VkImageView depthStencilImageView = VK_NULL_HANDLE;
		VkExtent2D extent{};
	};

	class VulkanDevice
	{
		constexpr static uint32   vulkanVersion = { VK_API_VERSION_1_4 };
		constexpr static uint32   MaxFramesInFlight = { 2 };

		constexpr static VkFormat depthStencilFormat = { VK_FORMAT_D32_SFLOAT_S8_UINT };

	public:
		VulkanDevice(SDL_Window* window, uint32 width, uint32 height);
		~VulkanDevice();

	public:
		VkInstance GetInstance() const { return vulkanInstance; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkDevice GetDevice() const { return device; }
		VkQueue GetGraphicsQueue() const { return gfxQueue; }
		uint32 GetGraphicsQueueFamilyIndex() const { return gfxQueueFamilyIndex; }
		VkSurfaceKHR GetSurface() const { return vulkanSurface; }
		VmaAllocator GetVmaAllocator() const { return vmaAllocator;  }
		VulkanSwapchain& GetSwapchain() { return *swapchain; }
		const VulkanSwapchain& GetSwapchain() const { return *swapchain; }
		VkFormat GetDepthStencilFormat() const { return depthStencilFormat; }
		VkImage GetDepthStencilImage() const { return depthStencilImage; }
		VkImageView GetDepthStencilImageView() const { return depthStencilImageView; }

		RenderFrameContext GetCurrentRenderFrameContext() const;

	public:
		// 현재 사용가능한 이미지인덱스를 PresentEngine에서 가져온다.
		bool AcquireNextImage();
		// false이면 재생성이 보류되었으므로 이번 프레임을 건너뛴다.
		bool UpdateSwapchain(uint32 width, uint32 height);

		void BeginFrame();
		void EndFrame();

	private:
		FrameResource& GetCurrentFrameResource() { return frameResources[frameIndex % MaxFramesInFlight]; }
		const FrameResource& GetCurrentFrameResource() const { return frameResources[frameIndex % MaxFramesInFlight]; }

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
		bool CreateDepthStencilResources();
		void DestroyDepthStencilResources();


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
		uint32 gfxQueueFamilyIndex = { UINT32_MAX } ;
		VkQueue gfxQueue = { VK_NULL_HANDLE };

		// Vulkan Memory Allocater
		VmaAllocator vmaAllocator = { nullptr };

		// Swapchain
		std::unique_ptr<VulkanSwapchain> swapchain;
		bool bRequireRecreateSwapchain = { false };
		uint32 imageIndex = { 0 };

		// Default depthstencil buffer
		VkImage depthStencilImage = VK_NULL_HANDLE;
		VkImageView depthStencilImageView = VK_NULL_HANDLE;
		VmaAllocation depthStencilImageAllocation = nullptr;

		// Frame and synchroniztion resources
		VkSemaphore timelineSemaphore = { nullptr };
		std::array<FrameResource, MaxFramesInFlight> frameResources;
		uint64 frameIndex = { 0 };
		uint64 nextSignalValue = { MaxFramesInFlight + 1 };
	};
}
