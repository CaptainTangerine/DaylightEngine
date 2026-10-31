#pragma once

#include <chrono>

struct SDL_Window;

namespace Dlight
{
	class  VulkanDevice;
	class  Renderer;
	class  ResourceManager;

	class Engine
	{
	public:
		Engine();
		~Engine();

	public:
		void Run();

	private:
		void Initialize();
		void Shutdown();
		void ShowError(const std::string& message) const;
		void SetWindowIcon();
		void ResetFrameStats();
		void UpdateDeltaTime();
		void UpdateFrameStats();

	private:
		// Window
		SDL_Window* window = { nullptr } ;
		uint32 width = { 1280 };
		uint32 height = { 720 };
		bool	 bRunning = { false };

		// 프레임 전체의 CPU/present 경과 시간. 게임 업데이트에서 사용할 수 있도록 초 단위로 저장한다.
		using FrameClock = std::chrono::steady_clock;
		FrameClock::time_point previousFrameTime = {};
		FrameClock::time_point logSampleStart = {};
		float deltaTime = { 0.0f };

		std::unique_ptr<VulkanDevice> vulkanDevice = { nullptr };
		std::unique_ptr<ResourceManager> resourceManager = { nullptr };
		std::unique_ptr<Renderer>	  renderer = { nullptr };

	};
}
