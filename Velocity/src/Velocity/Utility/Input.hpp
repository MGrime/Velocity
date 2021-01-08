#pragma once

#include <memory>

namespace Velocity
{
	class Input
	{
	public:
		static bool IsKeyPressed(int keyCode);
		static bool IsMouseButtonPressed(int keyCode);

		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();

	};
}