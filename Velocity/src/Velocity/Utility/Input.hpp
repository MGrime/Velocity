#pragma once

#include <memory>

#include "Velocity/Core/Events/KeyEvent.hpp"

namespace Velocity
{
	// Tracks input seperately, only for the keys that get used
	// They are inserted into the map when an event for that key comes in
	// This is because GLFW_REPEAT isn't exposed to this level, only the keycallback
	// So this singleton cannot process held vs pressed correctly by polling directly
	class Input
	{
	public:
		static bool IsKeyPressed(int keyCode);
		static bool IsKeyHeld(int keyCode);
		static bool IsMouseButtonPressed(int keyCode);

		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();

		static void OnKeyPressedEvent(KeyPressedEvent& e);
		static void OnKeyReleasedEvent(KeyReleasedEvent& e);
		
		// Called by renderer. Clears seen keys for this frame
		static void OnFrameFinished();

	
	private:
		static std::unordered_map<int, int> s_KeyMap;

	};
}
