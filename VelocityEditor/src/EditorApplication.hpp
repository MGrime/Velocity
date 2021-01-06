#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Velocity.hpp>

#include "Velocity/Renderer/Texture.hpp"

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
		auto& renderer = Velocity::Renderer::GetRenderer();
		
		m_CentralSquare = renderer->LoadMesh(m_Verts,m_Indices);

		auto transform = rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		
		renderer->AddStatic(m_CentralSquare, transform);

		m_Verts.at(0).Position = { -0.3f, -0.3f,0.0f };
		m_Verts.at(1).Position = { 0.3f, -0.3f,0.0f };
		m_Verts.at(2).Position = { 0.3f, 0.3f,0.0f };
		m_Verts.at(3).Position = { -0.3f, 0.3f,0.0f };

		m_Verts.at(0).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(1).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(2).Color = { 1.0f,1.0f,1.0f };
		m_Verts.at(3).Color = { 1.0f,1.0f,1.0f };

		auto square2 = renderer->LoadMesh(m_Verts, m_Indices);

		transform = rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		
		renderer->AddStatic(square2, transform);

		// TODO: TEST CODE REMOVE
		auto texture = renderer->CreateTexture("assets/textures/logo.png");

		VEL_CLIENT_INFO("Loaded texture {0} X:{1} Y:{2}", texture->GetPath(), texture->GetHeight(), texture->GetWidth());

		delete texture;
	}

	void OnUpdate() override
	{
		// Dont update when paused. TODO TEMPORARY
		if (Velocity::Application::GetWindow()->IsPaused())
		{
			return;
		}
		
		const glm::mat4 view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		const float width = static_cast<float>(Velocity::Application::GetWindow()->GetWidth());
		const float height = static_cast<float>(Velocity::Application::GetWindow()->GetHeight());
		
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), width/height, 0.1f, 10.0f);
		proj[1][1] *= -1.0f;
		
		Velocity::Renderer::GetRenderer()->BeginScene(view,proj);

		// Nothing dynmaic yet
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		auto transform = translate(glm::mat4(1.0f), glm::vec3(1.0f,0.0f,1.0f)) * rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		Velocity::Renderer::GetRenderer()->DrawDynamic(m_CentralSquare, transform);

		transform *= translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, -2.0f));

		Velocity::Renderer::GetRenderer()->DrawDynamic(m_CentralSquare, transform);

		Velocity::Renderer::GetRenderer()->EndScene();
		
	}

private:
	std::vector<Velocity::Vertex> m_Verts = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
	};

	Velocity::BufferManager::Renderable m_CentralSquare;
	
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

