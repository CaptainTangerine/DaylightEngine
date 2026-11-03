#pragma once

#include <chrono>
#include <memory>
#include <string>
#include "CoreTypes.h"

struct SDL_Window;

namespace Dlight
{
	class Engine;

	class EditorApplication
	{
	public:
		EditorApplication();
		~EditorApplication();

		EditorApplication(const EditorApplication&) = delete;
		EditorApplication& operator=(const EditorApplication&) = delete;

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
		SDL_Window* window = { nullptr };
		uint32 width = { 1280 };
		uint32 height = { 720 };
		bool bRunning = { false };

		// 프레임 전체의 CPU/present 경과 시간을 초 단위로 저장한다.
		using FrameClock = std::chrono::steady_clock;
		FrameClock::time_point previousFrameTime = {};
		FrameClock::time_point logSampleStart = {};
		float deltaTime = { 0.0f };

		std::unique_ptr<Engine> engine = { nullptr };
	};
}
