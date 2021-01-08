#pragma once

#include <string>
#include <Velocity/Core/Window.hpp>
#include <Velocity/Core/Events/ApplicationEvent.hpp>

#include <Velocity/Core/Layers/LayerStack.hpp>

#include <Velocity/ImGui/ImGuiLayer.hpp>

#include <Velocity/Utility/Timer.hpp>

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

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		
		// Events
		bool OnWindowClose(WindowCloseEvent& e);

		bool OnWindowResize(WindowResizeEvent& e);
		
		static std::shared_ptr<Window>& GetWindow()
		{
			return s_Window;
		}

	private:
		// Window is a singleton init by Application
		// This is only done as Application::GetWindow() makes sense
		static std::shared_ptr<Window>		 s_Window;

		// Store a reference to the renderer to save calling GetRenderer() all the time, even though we could
		std::shared_ptr<Renderer>			 r_Renderer;

		bool m_Running = true;

		// Stores layer implementions of the users app so we can update and dispatch events
		LayerStack m_LayerStack;

		// Always have a gui layer
		ImGuiLayer* m_ImGuiLayer;

		FrameTimer m_Timer;

	};

	// To be defined in client
	Application* CreateApplication();

}


