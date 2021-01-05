#pragma once

#include <Velocity.hpp>

class TestLayer : public Velocity::Layer
{
public:
	TestLayer() : Layer("Test Layer"){}
	
	void OnAttach() override
	{
		VEL_CLIENT_TRACE("Attaching layer {0}", GetName());
	}
	void OnDetach() override
	{
		VEL_CLIENT_TRACE("Detaching Layer {0}", GetName());
	};
	void OnUpdate() override
	{
		VEL_CLIENT_TRACE("Layer {0} was updated!", GetName());
	}
	void OnGuiRender() override
	{
		VEL_CLIENT_TRACE("Layer {0} drew some GUI", GetName());
	}
	void OnEvent(Velocity::Event& event) override
	{
		VEL_CLIENT_TRACE("Layer {0} received event {1}", GetName(), event.ToString());
	}
};

class EditorApplication : public Velocity::Application
{
public:
	EditorApplication() : Application("Velocity Editor",1280u,720u) 
	{
		// Set the editor icon
		GetWindow()->SetWindowIcon("assets/textures/logo.png");

		//PushLayer(new TestLayer());
	}

	~EditorApplication() = default;

};

