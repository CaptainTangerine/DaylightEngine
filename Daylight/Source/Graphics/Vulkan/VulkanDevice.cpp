#include "pch.h"
#include "VulkanDevice.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

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

	bool VulkanDevice::Initialize(SDL_Window* window)
	{
		if (!InitializeVulkan())
		{
			DL_LOG_ERROR("can't create a vulkan instance");
			return false;
		}

		if (!InitializeSurface(window))
		{
			DL_LOG_ERROR("Can't create a vulkansurface");
			return false;
		}

		// 창과 VulkanSurface를 어떤 GPU로 쓸지
		physicalDevice = FindPhysicalDevice();
		if (!physicalDevice)
		{
			DL_LOG_ERROR("Can't find a physical device");
			return false;
		}

		if (!FindGraphicsQueue())
		{
			DL_LOG_ERROR("Can't find a Compatible graphics queue");
			return false;
		}

		if (!CreateDevice())
		{
			DL_LOG_ERROR("Can't create a logical device or get its graphics queue");
			return false;
		}
		return true;
	}

	void VulkanDevice::Shutdown()
	{
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
		appInfo.apiVersion = VK_API_VERSION_1_4;

		// VulkanSurface를 만들기 위해 확장자들을 SDL에서 가져옴
		uint32 instExtCount = 0;
		const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&instExtCount);

		std::vector<const char*> requestedExtensions
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		for (uint32 i = 0; i < instExtCount; ++i)
		{
			requestedExtensions.push_back(extensions[i]);
		}

		// 실행중에 어떤 레이어를 활성화 할지 Vulkan에 알려줘야한다.
		std::vector<const char*> requestedLayers
		{
			// Validation Layer
			"VK_LAYER_KHRONOS_validation"
		};

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
		InstCreatInfo.pNext = &debugInfo;
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
		// Presentation?? Surface??
		for (size_t curFamilyIndex = 0; curFamilyIndex < queueFamilyProps.size(); ++curFamilyIndex)
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
		devCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
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
}
