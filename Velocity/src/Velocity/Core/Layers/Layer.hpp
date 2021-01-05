#pragma once

#include "Velocity/Core/Events/Event.hpp"

namespace Velocity
{
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer") : m_DebugName(name) {}

		virtual ~Layer() = default;

		// Called on pushing to stack
		virtual void OnAttach() {}

		// Called on popping from stack
		virtual void OnDetach() {}

		// Called when layer is updated
		virtual void OnUpdate() {}

		// Called each frame to draw GUI
		virtual void OnGuiRender(){}

		// Callback for events
		virtual void OnEvent(Event& event) {}

		const std::string& GetName() const { return m_DebugName; }
	
	private:
		std::string m_DebugName;
	};
}