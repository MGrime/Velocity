#include "Velocity/Core/EntryPoint.hpp"
#include "Velocity.hpp"

using namespace Velocity;

class VelocityDemo : public Application
{
public:
	VelocityDemo() : Application("Velocity Demo", 1280u, 720u)
	{
		// Set the editor icon
		GetWindow()->SetWindowIcon("assets/textures/logo.png");

		Renderer::GetRenderer()->ToggleGUI();
	}

	~VelocityDemo() = default;

};

// This is used in main defined by Velocity
Application* Velocity::CreateApplication()
{
	return new VelocityDemo();
}