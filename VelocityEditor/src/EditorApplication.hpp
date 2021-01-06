#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

		Velocity::Renderer::GetRenderer()->AddStatic(square, glm::mat4());

		m_Verts.at(0).Position = { -0.3f, -0.3f,0.0f };
		m_Verts.at(1).Position = { 0.3f, -0.3f,0.0f };
		m_Verts.at(2).Position = { 0.3f, 0.3f,0.0f };
		m_Verts.at(3).Position = { -0.3f, 0.3f,0.0f };

		m_Verts.at(0).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(1).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(2).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(3).Color = { 1.0f,1.0f,1.0f };

		auto square2 = Velocity::Renderer::GetRenderer()->LoadMesh(m_Verts, m_Indices);

		Velocity::Renderer::GetRenderer()->AddStatic(square2, glm::mat4());
	}

	void OnUpdate() override
	{
		const glm::mat4 view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		const float width = static_cast<float>(Velocity::Application::GetWindow()->GetWidth());
		const float height = static_cast<float>(Velocity::Application::GetWindow()->GetHeight());
		
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), width/height, 0.1f, 10.0f);
		proj[1][1] *= -1.0f;
		
		Velocity::Renderer::GetRenderer()->BeginScene(view,proj);

		// Nothing dynmaic yet

		Velocity::Renderer::GetRenderer()->EndScene();
		
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

