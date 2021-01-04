#pragma once

#include <string>
#include <Velocity/Core/Window.hpp>
#include <Velocity/Core/Events/ApplicationEvent.hpp>

struct GLFWwindow;

namespace Velocity 
{
	class Renderer;

	class Application
	{
	public:
		Application(const std::string& windowTitle, const uint32_t width = 800u, const uint32_t height = 600u);
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);
		
		// Events
		bool OnWindowClose(WindowCloseEvent& e);


	private:
		std::unique_ptr<Window> m_Window;
		std::unique_ptr<Renderer>* m_Renderer;

		bool m_Running = true;
	};

	// To be defined in client
	Application* CreateApplication();

}


