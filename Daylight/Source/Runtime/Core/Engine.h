#pragma once

#include <memory>
#include "CoreTypes.h"

struct SDL_Window;

namespace Dlight
{
	class VulkanDevice;
	class Renderer;
	class ResourceManager;

	class Engine
	{
	public:
		// window는 비소유 참조다. Engine보다 오래 살아 있어야 한다.
		Engine(SDL_Window* window, uint32 width, uint32 height);
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

	public:
		// false이면 스왑체인 재생성이 보류되었으므로 이번 프레임을 건너뛴다.
		bool PrepareFrame(uint32 width, uint32 height);
		bool AcquireNextImage();
		// 이미지 획득 성공 후 호출한다.
		void RenderFrame();

	private:
		void Initialize(SDL_Window* window, uint32 width, uint32 height);
		void Shutdown();

	private:
		std::unique_ptr<VulkanDevice> vulkanDevice = { nullptr };
		std::unique_ptr<ResourceManager> resourceManager = { nullptr };
		std::unique_ptr<Renderer> renderer = { nullptr };
	};
}
