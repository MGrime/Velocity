#pragma once

#define GLM_FORCE_RADIANS

#include <Velocity.hpp>

#include "SceneLayer.hpp"

class EditorApplication : public Velocity::Application
{
public:
	EditorApplication() : Application("Velocity Editor",1280u,720u) 
	{
		// Set the editor icon
		GetWindow()->SetWindowIcon("assets/textures/logo.png");

		PushLayer(new SceneLayer());
	}

	~EditorApplication() = default;

};

