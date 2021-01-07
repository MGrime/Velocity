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

		// Push the gui layer
		m_ImGuiLayer = new ImGuiLayer();
		m_LayerStack.PushOverlay(m_ImGuiLayer);
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
			// begin gui
			m_ImGuiLayer->Begin();

			// Render each layer
			for (Layer* layer : m_LayerStack)
			{
				layer->OnGuiRender();
			}
			// end gui
			m_ImGuiLayer->End();

			// Now update window
			s_Window->OnUpdate();
			
			// Render everything that has been submitted this frame

			if (!s_Window->IsPaused())
			{
				r_Renderer->Render();
			}
		}

		r_Renderer->Finalise();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
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

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 && e.GetHeight() == 0)
		{
			return true;
		}
		r_Renderer->OnWindowResize();
		return true;
	}

	Application::~Application()
	{
	}
}