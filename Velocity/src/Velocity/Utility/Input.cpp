#include "velpch.h"
#include "Input.hpp"

#include "GLFW/glfw3.h"
#include "Velocity/Core/Application.hpp"

namespace Velocity
{
	// Create key map
	std::unordered_map<int, int> Input::s_KeyMap = std::unordered_map<int, int>();
	
	bool Input::IsKeyHeld(int keyCode)
	{
		return s_KeyMap[keyCode] == GLFW_PRESS || s_KeyMap[keyCode] == GLFW_REPEAT;
	}

	bool Input::IsKeyPressed(int keyCode)
	{
		return s_KeyMap[keyCode] == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(int keyCode)
	{
		auto window = Application::GetWindow()->GetNative();
		auto state = glfwGetMouseButton(window, keyCode);

		return state == GLFW_PRESS;
		
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		auto window = Application::GetWindow()->GetNative();
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return std::pair<float, float>(static_cast<float>(xpos), static_cast<float>(ypos));
		
	}
	

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return y;
	}

	void Input::OnKeyPressedEvent(KeyPressedEvent& e)
	{		
		if (s_KeyMap[e.GetKeyCode()] == GLFW_PRESS || s_KeyMap[e.GetKeyCode()] == GLFW_REPEAT)
		{
			s_KeyMap[e.GetKeyCode()] = GLFW_REPEAT;
		}
		else
		{
			s_KeyMap[e.GetKeyCode()] = GLFW_PRESS;
		}
	}
	void Input::OnKeyReleasedEvent(KeyReleasedEvent& e)
	{
		s_KeyMap[e.GetKeyCode()] = GLFW_RELEASE;
	}
	void Input::OnFrameFinished()
	{
	}
}
