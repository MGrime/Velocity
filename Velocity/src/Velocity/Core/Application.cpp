#include "velpch.h"
#include "Application.hpp"

#include <GLFW/glfw3.h>

#include "Velocity/Core/Events/ApplicationEvent.hpp"
#include "Velocity/Core/Log.hpp"

#include "Velocity/Renderer/Renderer.hpp"

namespace Velocity
{
	std::shared_ptr<Window> Application::s_Window = nullptr;
	
	Application::Application(const std::string& windowTitle,const uint32_t width, const uint32_t height)
	{
		s_Window = std::make_shared<Window>(WindowProps( windowTitle,width,height ));
		s_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		// This will init everything
		r_Renderer = Renderer::GetRenderer();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			// First go through layer stack and update
			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}

			// TODO: GUI HERE

			// Now update window
			s_Window->OnUpdate();
			
			// Render everything that has been submitted this frame
			r_Renderer->Render();
		}

		r_Renderer->Finalise();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled())
				break;
		}
		

	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
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