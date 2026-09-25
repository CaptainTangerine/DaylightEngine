#pragma once

struct SDL_Window;

namespace Dlight
{
	class  VulkanDevice;

	class Engine
	{
	public:
		Engine();
		~Engine();

	public:
		bool Initialize();
		void Run();
		void Shutdown();

	private:
		void ShowError(const std::string& message) const;

	private:
		// Window
		SDL_Window* window = { nullptr } ;
		uint32 width = { 1920 };
		uint32 height = { 1080 };
		bool	 bRunning = { false };

		std::unique_ptr<VulkanDevice> vulkanDevice = { nullptr };
	};
}
