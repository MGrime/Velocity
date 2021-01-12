#include "velpch.h"
#include "Application.hpp"

#include <GLFW/glfw3.h>


#include "imgui.h"
#include "Velocity/Core/Events/ApplicationEvent.hpp"
#include "Velocity/Core/Log.hpp"

#include "Velocity/Renderer/Renderer.hpp"
#include "Velocity/Utility/Input.hpp"

namespace Velocity
{
	std::shared_ptr<Window> Application::s_Window = nullptr;
	
	Application::Application(const std::string& windowTitle,const uint32_t width, const uint32_t height)
	{
		s_Window = std::make_shared<Window>(WindowProps( windowTitle,width,height ));
		s_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		// This will init everything
		r_Renderer = Renderer::GetRenderer();

		// Push the gui layer
		m_ImGuiLayer = new ImGuiLayer();

		m_Timer = FrameTimer();
	}

	void Application::Run()
	{
		while (m_Running)
		{
			const Timestep time = m_Timer.GetDeltaTime();
			
			// First go through layer stack and update
			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(time);
			}

			// TODO: safely disable this code when gui rendering disabled

			// begin gui
			m_ImGuiLayer->Begin();

			// Disable the bulk of the renderering logic
			// Disabling the full imgui stack would require some difficult engineering on the vulkan side for little gain
			if (Renderer::GetRenderer()->m_EnableGUI)
			{
				// Render each layer
				for (Layer* layer : m_LayerStack)
				{
					layer->OnGuiRender();
				}
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
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(Application::OnKeyPressedEvent));
		dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(Application::OnKeyReleasedEvent));
		

		// Dispatch to imgui layer first
		m_ImGuiLayer->OnEvent(e);
		
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
		static uint32_t resizeCount = 0;
		if (e.GetWidth() == 0 && e.GetHeight() == 0)
		{
			return false;
		}
		r_Renderer->OnWindowResize();
		VEL_CORE_INFO("Resizing {0}", resizeCount);

		++resizeCount;
		return false;
	}


	bool Application::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		Input::OnKeyPressedEvent(e);

		return false;
	}
	bool Application::OnKeyReleasedEvent(KeyReleasedEvent& e)
	{
		Input::OnKeyReleasedEvent(e);
		return false;
	}

	Application::~Application()
	{
	}
}
