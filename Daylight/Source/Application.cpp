#include "pch.h"
#include "Application.h"
#include "Logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>


namespace Dlight
{
	// Vulkan Instance에 콜백함수 제공할 수 있고
	// 그 콜백이 나중에 생길 문제에 대한 메시지를 받게 된다.
	VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback(
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

	void Application::ShowError(const std::string& message) const
	{
		DL_LOG_ERROR(message);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Daylight - Error", message.c_str(), window);
	}

	bool Application::Initialize()
	{
		if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
		{
			ShowError(SDL_GetError());
			return false;
		}

		window = SDL_CreateWindow(
			"Daylight",
			static_cast<int>(width),
			static_cast<int>(height),
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		if (!window)
		{
			ShowError(SDL_GetError());
			return false;
		}

		DL_LOG_INFO("Window created: ", width, "x", height);

		if (!InitializeVulkan())
		{
			ShowError("can't create a vulkan instance");
			return false;
		}

		if (!InitializeSurface())
		{
			ShowError("Can't create a vulkansurface");
			return false;
		}

		// 창과 VulkanSurface를 어떤 GPU로 쓸지
		physicalDevice = FindPhysicalDevice();
		if (!physicalDevice)
		{
			ShowError("Can't find a physical device");
			return false;
		}

		if (!findGraphicsQueue())
		{
			ShowError("Can't find a Compatible graphics queue");
			return false;
		}
		return true;
	}

	void Application::Run()
	{
		bRunning = true;
		while (bRunning)
		{
			SDL_Event event{ 0 };

			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_EVENT_QUIT)
				{
					bRunning = false;
					break;
				}
				else if (event.type == SDL_EVENT_WINDOW_RESIZED)
				{
					width = event.window.data1;
					height = event.window.data2;
					break;
				}

			}
		}
	}

	void Application::Shutdown()
	{
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

		volkFinalize();

		if (window)
		{
			SDL_DestroyWindow(window);
		}

		SDL_Quit();
	}

	bool Application::InitializeVulkan()
	{
		// Volk : Vulkan Instance로부터 Vulkan함수 포인터를 로드해주는 라이브러리
		if (VK_SUCCESS != volkInitialize())
		{
			ShowError("Error Initializeing Volk");
			return false;
		}

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
		// 원하는 DebugCallback 함수와 어떤 종류나 심각도 수전을 원하는지
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
			return false;
		}

		volkLoadInstance(vulkanInstance);
		return true;
	}

	bool Application::InitializeSurface()
	{
		if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &vulkanSurface))
		{
			return false;
		}
		return true;
	}

	VkPhysicalDevice Application::FindPhysicalDevice() const
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

	bool Application::findGraphicsQueue()
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
}
