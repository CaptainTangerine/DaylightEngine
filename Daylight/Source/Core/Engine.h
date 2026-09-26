#pragma once

struct SDL_Window;

namespace Dlight
{
	class  VulkanDevice;
	class  Renderer;

	class Engine
	{
	public:
		Engine();
		~Engine();

	public:
		void Run();

	private:
		void Shutdown();
		void ShowError(const std::string& message) const;

	private:
		// Window
		SDL_Window* window = { nullptr } ;
		uint32 width = { 1920 };
		uint32 height = { 1080 };
		bool	 bRunning = { false };

		std::unique_ptr<VulkanDevice> vulkanDevice = { nullptr };
		std::unique_ptr<Renderer>	  renderer = { nullptr };
	};
}
