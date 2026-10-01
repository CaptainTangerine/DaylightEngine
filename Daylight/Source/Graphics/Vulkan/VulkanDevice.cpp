#include "pch.h"

#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "VulkanDevice.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "VulkanSwapchain.h"

namespace Dlight
{
	// Vulkan Instance에 콜백함수 제공할 수 있고
	// 그 콜백이 나중에 생길 문제에 대한 메시지를 받게 된다.
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		(void)messageType;
		(void)pUserData;

		const char* message =
			(pCallbackData && pCallbackData->pMessage)
			? pCallbackData->pMessage
			: "Vulkan validation message was empty.";

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			DL_LOG_ERROR("Vulkan validation: ", message);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			DL_LOG_WARN("Vulkan validation: ", message);
		}
		else
		{
			DL_LOG_INFO("Vulkan validation: ", message);
		}

		return VK_FALSE;
	}


	VulkanDevice::VulkanDevice(SDL_Window* window, uint32 width, uint32 height)
	{
		Initialize(window, width, height);
	}

	VulkanDevice::~VulkanDevice()
	{
		Shutdown();
	}

	bool VulkanDevice::AcquireNextImage()
	{
		// flight in frame 타임라인 세마포어를 wait

		// GPU가 해당 작업을 마쳤을때의 타임라인 세마포어 value
		const uint64 signalValue = nextSignalValue;
		// 현재 CPU가 GPU가 읽기를 마칠떄까지 기다려야하는 세마포어 value
		const uint64 waitValue = signalValue - MaxFramesInFlight;

		VkSemaphoreWaitInfo waitInfo = {};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores = &timelineSemaphore;
		waitInfo.pValues = &waitValue;

		const VkResult waitResult = vkWaitSemaphores(device, &waitInfo, UINT64_MAX);
		if (waitResult != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to wait for frame resources: ", waitResult);
			std::abort();
		}

		FrameResource& res = GetCurrentFrameResource();
		const VkResult resetResult = vkResetCommandPool(device, res.commandPool, 0);
		if (resetResult != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to reset command pool: ", resetResult);
			std::abort();
		}

		// 현재 프레임의 바이너리 세마포어
		VkSemaphore imageAquireSemaphore = res.imageAcquiredSemaphore;

		// present engine에 이미지 요청
		VkResult aquireResult = vkAcquireNextImageKHR(device, swapchain->GetSwapchain(), UINT64_MAX, imageAquireSemaphore, VK_NULL_HANDLE ,&imageIndex);

		// 리사이즈나 오래된 이미지 경우
		if (VK_ERROR_OUT_OF_DATE_KHR == aquireResult)
		{
			bRequireRecreateSwapchain = true;
			return false;
		}
		else if (VK_SUBOPTIMAL_KHR == aquireResult)
		{
			bRequireRecreateSwapchain = true;
			return true;
		}

		if (aquireResult != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to acquire swapchain image: ", aquireResult);
			std::abort();
		}

		return true;
	}

	void VulkanDevice::UpdateSwapchain(uint32 width, uint32 height)
	{
		if (!bRequireRecreateSwapchain)
		{
			return;
		}

		if (width == 0 || height == 0)
		{
			return;
		}

		vkDeviceWaitIdle(device);
		swapchain->Shutdown();
		swapchain->Initialize(width, height);

		if (!CreateDepthStencilResources())
		{
			DL_LOG_ERROR("Failed to recreate depth-stencil resources");
			std::abort();
		}

		bRequireRecreateSwapchain = false;
	}

	RenderFrameContext VulkanDevice::GetCurrentRenderFrameContext() const
	{
		RenderFrameContext context{};
		context.commandBuffer = GetCurrentFrameResource().commandBuffer;
		context.colorImageView = swapchain->GetSwapchainImageView(imageIndex);
		context.depthStencilImageView = depthStencilImageView;
		context.extent = { swapchain->GetWidth(), swapchain->GetHeight() };
		return context;
	}

	void VulkanDevice::BeginFrame()
	{
		FrameResource& res = GetCurrentFrameResource();

		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// 기존 기록을 다음 프레임에는 사용하지않는다.
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (VK_SUCCESS != vkBeginCommandBuffer(res.commandBuffer, &cmdBeginInfo))
		{
			DL_LOG_ERROR("Failed to begin command buffer: ");
			std::abort();
		}

		std::vector<VkImageMemoryBarrier2> layoutBarriers;
		layoutBarriers.resize(2);

		// Color Attachment
		layoutBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		layoutBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_NONE;
		layoutBarriers[0].srcAccessMask = 0;
		layoutBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		layoutBarriers[0].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		layoutBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		layoutBarriers[0].image = swapchain->GetSwapchianImage(imageIndex);
		layoutBarriers[0].subresourceRange.aspectMask = { VK_IMAGE_ASPECT_COLOR_BIT };
		layoutBarriers[0].subresourceRange.baseMipLevel = 0;
		layoutBarriers[0].subresourceRange.levelCount = 1;
		layoutBarriers[0].subresourceRange.baseArrayLayer = 0;
		layoutBarriers[0].subresourceRange.layerCount = 1;

		// Depthstencil
		layoutBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		layoutBarriers[1].srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		layoutBarriers[1].srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layoutBarriers[1].dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		layoutBarriers[1].dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		layoutBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		layoutBarriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		layoutBarriers[1].image = depthStencilImage;
		layoutBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		layoutBarriers[1].subresourceRange.baseMipLevel = 0;
		layoutBarriers[1].subresourceRange.levelCount = 1;
		layoutBarriers[1].subresourceRange.baseArrayLayer = 0;
		layoutBarriers[1].subresourceRange.layerCount = 1;

		VkDependencyInfo depInfo = {};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = static_cast<uint32>(layoutBarriers.size());
		depInfo.pImageMemoryBarriers = layoutBarriers.data();
		vkCmdPipelineBarrier2(res.commandBuffer, &depInfo);


	}

	void VulkanDevice::EndFrame()
	{
		FrameResource& res = GetCurrentFrameResource();
		const VkSemaphore renderComplete = swapchain->GetRendercompleteSemaphore(imageIndex);
		const VkSwapchainKHR swapchainHandle = swapchain->GetSwapchain();

		VkImageMemoryBarrier2 presentLayoutBarrier = {};
		presentLayoutBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		presentLayoutBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		presentLayoutBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		presentLayoutBarrier.dstStageMask = VK_PIPELINE_STAGE_NONE;
		presentLayoutBarrier.dstAccessMask = 0;
		presentLayoutBarrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		presentLayoutBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		presentLayoutBarrier.image = swapchain->GetSwapchianImage(imageIndex);
		presentLayoutBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentLayoutBarrier.subresourceRange.baseMipLevel = 0;
		presentLayoutBarrier.subresourceRange.levelCount = 1;
		presentLayoutBarrier.subresourceRange.baseArrayLayer = 0;
		presentLayoutBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo presentDepInfo = {};
		presentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		presentDepInfo.imageMemoryBarrierCount = 1;
		presentDepInfo.pImageMemoryBarriers = &presentLayoutBarrier;
		vkCmdPipelineBarrier2(res.commandBuffer, &presentDepInfo);

		vkEndCommandBuffer(res.commandBuffer);

		// 이후 present시 presentEngine과 바이너리 세마포어로 동기화 처리해줘야함
		// 현재 PresentEngien으로부터 이미지를 가져올 수 있는지 GPU에게 알려줘야한다.
		// imageAquire세마포어를 통해서 GPU가 써도 되는 순간이 오면 그 떄 signal

		// 이후 GPU 작업이 다 끝나면 타임라인 세마포어를 signal하고 나중에 fligt in Frame으로 CPU가 작업을 이어서 한다.

		// 다 끝나음을 이제 다시 presentEngine에 알려줘야한다.
		// completeSemphore로 wait하는 presentEngien은 이것을 보고 Presnet를 한다.

		VkSemaphoreSubmitInfo imageAquireWaitInfo = {};
		imageAquireWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		imageAquireWaitInfo.semaphore = res.imageAcquiredSemaphore;
		imageAquireWaitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		std::vector<VkSemaphoreSubmitInfo> semaphoreSignals;
		semaphoreSignals.resize(2);

		//render complete semaphore
		semaphoreSignals[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		semaphoreSignals[0].stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		semaphoreSignals[0].semaphore = renderComplete;

		// timellinesemaphore
		semaphoreSignals[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		semaphoreSignals[1].semaphore = timelineSemaphore;
		semaphoreSignals[1].value = nextSignalValue;
		semaphoreSignals[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		VkCommandBufferSubmitInfo cmdSubmitInfo = {};
		cmdSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdSubmitInfo.commandBuffer = res.commandBuffer;

		VkSubmitInfo2 submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &imageAquireWaitInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
		submitInfo.signalSemaphoreInfoCount = static_cast<uint32>(semaphoreSignals.size());
		submitInfo.pSignalSemaphoreInfos = semaphoreSignals.data();
		vkQueueSubmit2(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE);

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderComplete;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchainHandle;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		const VkResult presentResult = vkQueuePresentKHR(gfxQueue, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		{
			bRequireRecreateSwapchain = true;
		}
		// EndFrame에 따로 하는 것이유는 스왑체인 재생성시에는 Render->EndFrame을 하지 않기때문이다.
		++frameIndex;
		++nextSignalValue;
	}

	void VulkanDevice::Initialize(SDL_Window* window, uint32 width, uint32 height)
	{
		if (!InitializeVulkan())
		{
			DL_LOG_ERROR("can't create a vulkan instance");
			std::abort();
		}

		if (!InitializeSurface(window))
		{
			DL_LOG_ERROR("Can't create a vulkansurface");
			std::abort();
		}

		// 창과 VulkanSurface를 어떤 GPU로 쓸지
		physicalDevice = FindPhysicalDevice();
		if (!physicalDevice)
		{
			DL_LOG_ERROR("Can't find a physical device");
			std::abort();
		}

		if (!FindGraphicsQueue())
		{
			DL_LOG_ERROR("Can't find a Compatible graphics queue");
			std::abort();
		}

		if (!CreateDevice())
		{
			DL_LOG_ERROR("Can't create a logical device or get its graphics queue");
			std::abort();
		}

		if (!InitializeVMA())
		{
			DL_LOG_ERROR("Failed to initialize Vulkan memory allocator");
			std::abort();
		}

		swapchain = std::make_unique<VulkanSwapchain>(*this, width, height);

		if (!CreateDepthStencilResources())
		{
			DL_LOG_ERROR("Failed to create depth-stencil resources");
			std::abort();
		}

		if (!CreateSyncResources())
		{
			DL_LOG_ERROR("Can't create a Sync Resource");
			std::abort();
		}

		if (!CreateCommandBuffers())
		{
			DL_LOG_ERROR("Can't create command buffer objects");
			std::abort();
		}
	}

	void VulkanDevice::Shutdown()
	{
		DestroyDepthStencilResources();

		if (timelineSemaphore)
		{
			vkDestroySemaphore(device, timelineSemaphore, nullptr);
		}

		for (auto& res : frameResources)
		{
			vkDestroySemaphore(device, res.imageAcquiredSemaphore, nullptr);
			vkDestroyCommandPool(device, res.commandPool, nullptr);
		}

		if (swapchain)
		{
			swapchain.reset();
		}

		if (vmaAllocator)
		{
			vmaDestroyAllocator(vmaAllocator);
			vmaAllocator = nullptr;
		}

		if (device)
		{
			vkDestroyDevice(device, nullptr);
			device = VK_NULL_HANDLE;
		}

		if (vulkanSurface)
		{
			vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, nullptr);
			vulkanSurface = VK_NULL_HANDLE;
		}

		if (debugMessenger)
		{
			vkDestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
			debugMessenger = VK_NULL_HANDLE;
		}

		if (vulkanInstance)
		{
			vkDestroyInstance(vulkanInstance, nullptr);
			vulkanInstance = VK_NULL_HANDLE;
		}

		if (volkInitialized)
		{
			volkFinalize();
			volkInitialized = false;
		}

		physicalDevice = VK_NULL_HANDLE;
		gfxQueue = VK_NULL_HANDLE;
		gfxQueueFamilyIndex = UINT32_MAX;

	}

	bool VulkanDevice::InitializeVulkan()
	{
#ifdef NDEBUG
		bool useValidation = false;
#else
		bool useValidation = true;
#endif // !DEBUG

		// Volk : Vulkan Instance로부터 Vulkan함수 포인터를 로드해주는 라이브러리
		if (VK_SUCCESS != volkInitialize())
		{
			DL_LOG_ERROR("Error Initializeing Volk");
			return false;
		}
		volkInitialized = true;

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Daylight";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = vulkanVersion;

		// VulkanSurface를 만들기 위해 확장자들을 SDL에서 가져옴
		uint32 instExtCount = 0;
		const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&instExtCount);

		std::vector<const char*> requestedExtensions{};

		for (uint32 i = 0; i < instExtCount; ++i)
		{
			requestedExtensions.push_back(extensions[i]);
		}

		// 실행중에 어떤 레이어를 활성화 할지 Vulkan에 알려줘야한다.
		std::vector<const char*> requestedLayers{};

		if (useValidation)
		{
			requestedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			// Validation Layer
			requestedLayers.push_back("VK_LAYER_KHRONOS_validation");
		}

		// Vulkan Instance 생성 시 함께 넘길 Vulkan 구조체들
		// 원하는 DebugCallback 함수와 어떤 종류나 심각도 수준을 원하는지
		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = DebugCallback;

		// sTpye : 이 메모리가 어떤 vulkan의 구조체인지
		// pNext : 그 구조체의 추가 옵션을 링크드 리스트 형태로 관리
		VkInstanceCreateInfo InstCreatInfo{};
		InstCreatInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		if (useValidation)
		{
			InstCreatInfo.pNext = &debugInfo;
		}
		InstCreatInfo.pApplicationInfo = &appInfo;
		InstCreatInfo.enabledLayerCount = static_cast<uint32>(requestedLayers.size());
		InstCreatInfo.ppEnabledLayerNames = requestedLayers.data();
		InstCreatInfo.enabledExtensionCount = static_cast<uint32>(requestedExtensions.size());
		InstCreatInfo.ppEnabledExtensionNames = requestedExtensions.data();

		if (VK_SUCCESS != vkCreateInstance(&InstCreatInfo, nullptr, &vulkanInstance))
		{
			DL_LOG_ERROR("Failed to create Vulkan instance");
			return false;
		}

		volkLoadInstance(vulkanInstance);

		// pNext의 콜백은 인스턴스 생성/소멸 중에만 유효하므로 실행 중 사용할 messenger를 별도로 생성한다.
		if (useValidation)
		{
			const VkResult messengerResult = vkCreateDebugUtilsMessengerEXT(
				vulkanInstance, &debugInfo, nullptr, &debugMessenger);
			if (messengerResult != VK_SUCCESS)
			{
				DL_LOG_ERROR("Failed to create Vulkan debug messenger: ", messengerResult);
				return false;
			}
		}
		return true;
	}

	bool VulkanDevice::InitializeSurface(SDL_Window* window)
	{
		if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &vulkanSurface))
		{
			DL_LOG_ERROR("Failed to create Vulkan surface: ", SDL_GetError());
			return false;
		}
		return true;
	}

	VkPhysicalDevice VulkanDevice::FindPhysicalDevice() const
	{
		uint32 deviceCount = 0;
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			return VK_NULL_HANDLE;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

		// 첫 번째 GPU를 기본값으로 사용한다.
		VkPhysicalDevice selectedDevice = devices[0];

		// 외장 GPU가 있으면 우선해서 사용한다.
		for (VkPhysicalDevice device : devices)
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device, &properties);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				selectedDevice = device;
				break;
			}
		}

		VkPhysicalDeviceProperties selectedProperties{};
		vkGetPhysicalDeviceProperties(selectedDevice, &selectedProperties);
		DL_LOG_INFO("Selected Vulkan GPU: ", selectedProperties.deviceName);

		return selectedDevice;
	}

	bool VulkanDevice::FindGraphicsQueue()
	{
		uint32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties2> queueFamilyProps(queueFamilyCount, { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 });
		vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

		// 큐패밀리중에서 그래픽스에 해당하는 하드웨어를 줄 수 있는지 검사한다.
		for (uint32 curFamilyIndex = 0; curFamilyIndex < queueFamilyProps.size(); ++curFamilyIndex)
		{
			VkBool32 bHasPresentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, curFamilyIndex, vulkanSurface, &bHasPresentSupport);

			const auto& props = queueFamilyProps[curFamilyIndex];

			if ((props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) && bHasPresentSupport)
			{
				gfxQueueFamilyIndex = curFamilyIndex;
				return true;
			}
		}
		return false;
	}
	bool VulkanDevice::CreateDevice()
	{
		// Query Supported Features
		VkPhysicalDeviceVulkan14Features supportedFeatures14{};
		supportedFeatures14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		supportedFeatures14.pNext = nullptr;

		VkPhysicalDeviceVulkan13Features supportedFeatures13{};
		supportedFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		supportedFeatures13.pNext = &supportedFeatures14;

		VkPhysicalDeviceVulkan12Features supportedFeatures12{};
		supportedFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		supportedFeatures12.pNext = &supportedFeatures13;

		VkPhysicalDeviceFeatures2 supportedFeatures{};
		supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		supportedFeatures.pNext = &supportedFeatures12;

		vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);

		if (!supportedFeatures13.dynamicRendering ||
			!supportedFeatures13.synchronization2 ||
			!supportedFeatures12.timelineSemaphore)
		{
			DL_LOG_ERROR("Physical device doesn't meet the feature requirement");
			return false;
		}
		// 지원 여부만 확인하고 원하는 기능만 활성화한 Device를 가져오기 위해 세팅
		VkPhysicalDeviceVulkan14Features enabledFeatures14{};
		enabledFeatures14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		enabledFeatures14.pNext = nullptr;

		VkPhysicalDeviceVulkan13Features enabledFeatures13{};
		enabledFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		enabledFeatures13.pNext = &enabledFeatures14;
		enabledFeatures13.synchronization2 = VK_TRUE;
		enabledFeatures13.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceVulkan12Features enabledFeatures12{};
		enabledFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		enabledFeatures12.pNext = &enabledFeatures13;
		enabledFeatures12.timelineSemaphore = VK_TRUE;

		VkPhysicalDeviceFeatures2 enabledFeatures{};
		enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		enabledFeatures.pNext = &enabledFeatures12;

		std::vector<float> queuePriorities{ 1.f };
		VkDeviceQueueCreateInfo gfxQueueInfo{};
		gfxQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		gfxQueueInfo.queueFamilyIndex = gfxQueueFamilyIndex;
		gfxQueueInfo.queueCount = 1;
		gfxQueueInfo.pQueuePriorities = queuePriorities.data();

		const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo devCreateInfo{};
		devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		devCreateInfo.pNext = &enabledFeatures;
		devCreateInfo.queueCreateInfoCount = 1;
		devCreateInfo.pQueueCreateInfos = &gfxQueueInfo;
		devCreateInfo.enabledExtensionCount = static_cast<uint32>(deviceExtensions.size());
		devCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		devCreateInfo.pEnabledFeatures = nullptr;

		// GPU 자원에 접근하는데 사용될 VkDevice 객체생성
		if (VK_SUCCESS != vkCreateDevice(physicalDevice, &devCreateInfo, nullptr, &device))
		{
			DL_LOG_ERROR("Failed to create Vulkan logical device");
			return false;
		}

		vkGetDeviceQueue(device, gfxQueueFamilyIndex, 0, &gfxQueue);
		if (!gfxQueue)
		{
			DL_LOG_ERROR("Could't get the graphics queue");
			return false;
		}

		return true;
	}
	bool VulkanDevice::InitializeVMA()
	{
		VmaVulkanFunctions vmaFuncInfo{};
		VmaAllocatorCreateInfo vmaAllocInfo{};
		vmaAllocInfo.flags = 0;
		vmaAllocInfo.physicalDevice = physicalDevice;
		vmaAllocInfo.device = device;
		vmaAllocInfo.pVulkanFunctions = &vmaFuncInfo;
		vmaAllocInfo.instance = vulkanInstance;
		vmaAllocInfo.vulkanApiVersion = vulkanVersion;

		vmaImportVulkanFunctionsFromVolk(&vmaAllocInfo, &vmaFuncInfo);

		if (VK_SUCCESS != vmaCreateAllocator(&vmaAllocInfo, &vmaAllocator))
		{
			return false;
		}

		return true;
	}

	bool VulkanDevice::CreateDepthStencilResources()
	{
		VkFormatProperties formatProperties{};
		vkGetPhysicalDeviceFormatProperties(physicalDevice, depthStencilFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			DL_LOG_ERROR("VK_FORMAT_D32_SFLOAT_S8_UINT is not supported as a depth-stencil attachment");
			return false;
		}

		DestroyDepthStencilResources();

		VkImageCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depthStencilCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		depthStencilCreateInfo.format = depthStencilFormat;
		depthStencilCreateInfo.extent.width = GetSwapchain().GetWidth();
		depthStencilCreateInfo.extent.height = GetSwapchain().GetHeight();
		depthStencilCreateInfo.extent.depth = 1;
		depthStencilCreateInfo.mipLevels = 1;
		depthStencilCreateInfo.arrayLayers = 1;
		depthStencilCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		depthStencilCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		depthStencilCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthStencilCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (VK_SUCCESS != vmaCreateImage(vmaAllocator, &depthStencilCreateInfo, &allocInfo, &depthStencilImage, &depthStencilImageAllocation, nullptr))
		{
			DL_LOG_ERROR("Failed to allocate depth-stencil image");
			return false;
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthStencilImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthStencilFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &depthStencilImageView) != VK_SUCCESS)
		{
			DL_LOG_ERROR("Failed to create depth-stencil image view");
			DestroyDepthStencilResources();
			return false;
		}

		return true;
	}

	void VulkanDevice::DestroyDepthStencilResources()
	{
		if (depthStencilImageView)
		{
			vkDestroyImageView(device, depthStencilImageView, nullptr);
			depthStencilImageView = nullptr;
		}
		if (depthStencilImage)
		{
			vmaDestroyImage(vmaAllocator, depthStencilImage, depthStencilImageAllocation);
			depthStencilImage = nullptr;
			depthStencilImageAllocation = nullptr;
		}
	}

	bool VulkanDevice::CreateSyncResources()
	{
		VkSemaphoreTypeCreateInfo semaphoreTypeInfo = {};
		semaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		semaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		semaphoreTypeInfo.initialValue = MaxFramesInFlight;

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = &semaphoreTypeInfo;

		if (VK_SUCCESS != vkCreateSemaphore(device, &semaphoreInfo, nullptr, &timelineSemaphore))
		{
			DL_LOG_ERROR("Unable to create timeline semaphore");
			return false;
		}

		// per-frame imageacquire semaphores
		for (FrameResource& res : frameResources)
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (VK_SUCCESS != vkCreateSemaphore(device, &semaphoreInfo, nullptr, &res.imageAcquiredSemaphore))
			{
				DL_LOG_ERROR("Error creating the per-frame image-acquire semaphore");
				return false;
			}
		}

		return true;
	}
	bool VulkanDevice::CreateCommandBuffers()
	{
		for (FrameResource& res : frameResources)
		{
			VkCommandPoolCreateInfo poolInfo = {};

			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = gfxQueueFamilyIndex;

			if (VK_SUCCESS != vkCreateCommandPool(device, &poolInfo, nullptr, &res.commandPool))
			{
				DL_LOG_ERROR("Fail to create command buffer pool");
				return false;
			}

			VkCommandBufferAllocateInfo cmdAllocInfo = {};

			cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdAllocInfo.commandPool = res.commandPool;
			cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdAllocInfo.commandBufferCount = 1;

			if (VK_SUCCESS != vkAllocateCommandBuffers(device, &cmdAllocInfo, &res.commandBuffer))
			{
				DL_LOG_ERROR("Unable to allocate command buffer");
				return false;
			}
		}
		return true;
	}
}
