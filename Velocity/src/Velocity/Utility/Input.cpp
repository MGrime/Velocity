#include "velpch.h"
#include "Input.hpp"

#include "GLFW/glfw3.h"
#include "Velocity/Core/Application.hpp"

namespace Velocity
{
	bool Input::IsKeyPressed(int keyCode)
	{
		auto window = Application::GetWindow()->GetNative();
		auto state = glfwGetKey(window, keyCode);

		return state == GLFW_PRESS || state == GLFW_REPEAT;	
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
}
