#include "velpch.h"
#include "Application.hpp"

#include <GLFW/glfw3.h>

#include "Velocity/Core/Events/ApplicationEvent.hpp"
#include "Velocity/Core/Log.hpp"

#include "Velocity/Core/Renderer.hpp"

namespace Velocity
{
	Application::Application(const std::string& windowTitle,const uint32_t width, const uint32_t height)
	{
		m_Window = std::make_unique<Window>(WindowProps( windowTitle,width,height ));
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		// This will init everything
		m_Renderer = Renderer::GetRenderer();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	Application::~Application()
	{
	}
}