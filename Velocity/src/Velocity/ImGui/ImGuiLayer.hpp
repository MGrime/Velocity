#pragma once

#include <Velocity/Core/Layers/Layer.hpp>

#include <Velocity/Core/Events/MouseEvent.hpp>
#include <Velocity/Core/Events/KeyEvent.hpp>
#include <Velocity/Core/Events/ApplicationEvent.hpp>

namespace Velocity
{
	class ImGuiLayer : public Layer
	{
	public:
		void OnEvent(Event& event) override;
		
		void Begin();
		void End();
	private:
		bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
		bool OnMouseMovedEvent(MouseMovedEvent& e);
		bool OnMouseScrolledEvent(MouseScrolledEvent& e);
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnKeyReleasedEvent(KeyReleasedEvent& e);
		bool OnKeyTypedEvent(KeyTypedEvent& e);
	};
}
