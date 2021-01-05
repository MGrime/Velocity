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

class SceneLayer : public Velocity::Layer
{
public:
	SceneLayer() : Layer("Scene Layer") {}

	void OnAttach() override
	{
		auto square = Velocity::Renderer::GetRenderer()->LoadMesh(m_Verts,m_Indices);

		Velocity::Renderer::GetRenderer()->Submit(square);

		m_Verts.at(0).Position = { -0.3f, -0.3f,0.0f };
		m_Verts.at(1).Position = { 0.3f, -0.3f,0.0f };
		m_Verts.at(2).Position = { 0.3f, 0.3f,0.0f };
		m_Verts.at(3).Position = { -0.3f, 0.3f,0.0f };

		m_Verts.at(0).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(1).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(2).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(3).Color = { 1.0f,1.0f,1.0f };

		auto square2 = Velocity::Renderer::GetRenderer()->LoadMesh(m_Verts, m_Indices);

		Velocity::Renderer::GetRenderer()->Submit(square2);
	}
private:
	std::vector<Velocity::Vertex> m_Verts = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
	};

	std::vector<uint32_t> m_Indices = {
		0u,1u,2u,2u,3u,0u
	};
};

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

