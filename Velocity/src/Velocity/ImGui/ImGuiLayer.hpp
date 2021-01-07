#pragma once

#include <Velocity/Core/Layers/Layer.hpp>

namespace Velocity
{
	class ImGuiLayer : public Layer
	{
	public:
		void Begin();
		void End();
	};
}