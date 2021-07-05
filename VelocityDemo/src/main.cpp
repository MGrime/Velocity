#include "Velocity/Core/EntryPoint.hpp"
#include "Velocity.hpp"


#include "Layers/GameLayer.hpp"

using namespace Velocity;

class VelocityDemo : public Application
{
public:
	VelocityDemo() : Application("Velocity Demo Game - Space Invaders", 1280u, 720u)
	{
		// Add game layer
		PushLayer(new GameLayer());
	}

	~VelocityDemo() override = default;

};

// This is used in main defined by Velocity
Application* Velocity::CreateApplication()
{
	return new VelocityDemo();
}