#include "EditorLayer.hpp"


#include "../Panels/CameraStatePanel.hpp"
#include "../Panels/SceneViewPanel.hpp"
#include "../Panels/MainMenuPanel.hpp"
#include "Velocity/Utility/Input.hpp"

void EditorLayer::OnGuiRender()
{
	MainMenuPanel::Draw(m_Scene.get());
	SceneViewPanel::Draw(m_Scene.get());
	CameraStatePanel::Draw(*m_CameraController->GetCamera());
}

void EditorLayer::OnAttach()
{
	auto& renderer = Renderer::GetRenderer();

	// Wrap raw loaded scene into a unique ptr to link to lifespan of my application
	m_Scene = std::unique_ptr<Scene>(Scene::LoadScene("assets/scenes/default"));

	// Prepare camera
	m_CameraController = std::make_unique<DefaultCameraController>(m_Scene->GetCamera());

	m_Skybox = std::unique_ptr<Skybox>(renderer->CreateSkybox("assets/textures/skyboxes/Buddha",".jpg"));
	m_Scene->SetSkybox(m_Skybox.get());

	// Set our loaded scene
	Renderer::GetRenderer()->SetScene(m_Scene.get());

	//Renderer::GetRenderer()->ToggleGUI();
}

void EditorLayer::OnDetach()
{
	
}

void EditorLayer::OnUpdate(Timestep deltaTime)
{
	m_CameraController->OnUpdate(deltaTime);

	// Orbit light
	static float orbitTime = 0.0f;
	orbitTime += deltaTime * 0.8f;
	
	//m_Light.GetComponent<PointLightComponent>().Position = glm::vec3{} + glm::vec3(cos(orbitTime) * 8.0f, 0.0f, sin(orbitTime) * 8.0f);
}

void EditorLayer::OnEvent(Event& event)
{
	m_CameraController->OnEvent(event);
}
