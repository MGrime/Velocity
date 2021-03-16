#include "EditorLayer.hpp"


#include "../Panels/CameraStatePanel.hpp"
#include "../Panels/SceneViewPanel.hpp"
#include "../Panels/MainMenuPanel.hpp"
#include "Velocity/Utility/Input.hpp"

void EditorLayer::OnGuiRender()
{
	bool newSceneLoaded = false;
	std::string newScenePath = "";
	
	MainMenuPanel::Draw(m_Scene.get(), &newSceneLoaded, &newScenePath);
	// They have opened a new scene
	if (newSceneLoaded)
	{		
		Scene* newScene = Scene::LoadScene(newScenePath);
		
		// Release old scene and attach new scene
		m_Scene.reset(newScene);
		
		// Shift the camera controller to the new scene camera
		m_CameraController->SetCamera(newScene->GetCamera());

		// Set skybox
		m_Scene->SetSkybox(m_Skybox.get());

		// Set to render
		// TODO: Check if this could be moved to load logic itself
		Renderer::GetRenderer()->SetScene(m_Scene.get());
		
	}
	SceneViewPanel::Draw(m_Scene.get());
	CameraStatePanel::Draw(m_CameraController->GetCamera());
}

void EditorLayer::OnAttach()
{
	auto& renderer = Renderer::GetRenderer();

	m_Skybox = std::unique_ptr<Skybox>(renderer->CreateSkybox("assets/textures/skyboxes/Buddha",".jpg"));

	m_CameraController = std::make_unique<DefaultCameraController>(nullptr);

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
