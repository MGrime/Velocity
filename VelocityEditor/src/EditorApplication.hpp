#pragma once

#include <Velocity.hpp>

class EditorApplication : public Velocity::Application
{
public:
	EditorApplication() : Application("Velocity Editor",1280u,720u) 
	{
		// Set the editor icon
		GetWindow()->SetWindowIcon("assets/textures/logo.png");
	}

	~EditorApplication() = default;

};

