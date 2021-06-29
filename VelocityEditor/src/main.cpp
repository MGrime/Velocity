#include "Velocity/Core/EntryPoint.hpp"	
#include "Layers/EditorLayer.hpp"

using namespace Velocity;

class VelocityEditor : public Velocity::Application
{
public:
	VelocityEditor() : Application("Velocity Editor", 1280u, 720u)
	{
		// Set the editor icon
		GetWindow()->SetWindowIcon("assets/textures/logo.png");
		
		PushLayer(new EditorLayer);
	}

	~VelocityEditor() = default;

};

// This is used in main defined by Velocity
Application* Velocity::CreateApplication()
{
	return new VelocityEditor();
}