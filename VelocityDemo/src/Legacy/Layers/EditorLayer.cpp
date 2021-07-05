#include "EditorLayer.hpp"


#include "../Panels/CameraStatePanel.hpp"
#include "../Panels/GizmoControlPanel.hpp"
#include "../Panels/SceneViewPanel.hpp"
#include "../Panels/MainMenuPanel.hpp"
#include "Velocity/Utility/Input.hpp"

void EditorLayer::OnGuiRender()
{
	bool newSceneLoaded = false;
	bool sceneSaved = false;
	bool sceneClosed = false;
	std::string newScenePath = "";
	
	MainMenuPanel::Draw(m_Scene.get(), &newSceneLoaded, &newScenePath,&sceneSaved,&sceneClosed);
	// They have opened a new scene
	if (newSceneLoaded)
	{		
		Scene* newScene = Scene::LoadScene(newScenePath);
		
		// Release old scene and attach new scene
		m_Scene.reset(newScene);
		
		// Shift the camera controller to the new scene camera
		m_CameraController->SetCamera(newScene->GetCamera());

		// Set to render
		// TODO: Check if this could be moved to load logic itself
		Renderer::GetRenderer()->SetScene(m_Scene.get());

		// Update window title
		auto& window = Application::GetWindow();
		window->SetWindowTitle(window->GetBaseTitle() + " - Editing: " + m_Scene->GetSceneName());

		// Clear selected entity from config to avoid null crash
		SceneViewPanel::ClearSelectedEntity();
		Renderer::GetRenderer()->SetGizmoEntity(nullptr);
		
	}
	// They have saved the scene
	if (sceneSaved)
	{
		// Update window title
		auto& window = Application::GetWindow();
		window->SetWindowTitle(window->GetBaseTitle() + " - Editing: " + m_Scene->GetSceneName());
	}

	if (sceneClosed)
	{
		// Set scene to no render
		Renderer::GetRenderer()->SetScene(nullptr);

		// Clean up editor relating to scene
		m_Scene.reset(nullptr);
		m_CameraController->SetCamera(nullptr);

		// Reset title
		auto& window = Application::GetWindow();
		window->SetWindowTitle(window->GetBaseTitle());

		// Clear selected entity from config to avoid null crash
		SceneViewPanel::ClearSelectedEntity();
		Renderer::GetRenderer()->SetGizmoEntity(nullptr);
	}

	
	SceneViewPanel::Draw(m_Scene.get());
	CameraStatePanel::Draw(m_CameraController->GetCamera());
	GizmoControlPanel::Draw();
}

void EditorLayer::OnAttach()
{
	m_CameraController = std::make_unique<DefaultCameraController>(nullptr);

	// Set up gizmo
	GizmoControlPanel::Init();
	Renderer::GetRenderer()->SetGizmoMode(ImGuizmo::MODE::WORLD);
	Renderer::GetRenderer()->SetGizmoOperation(ImGuizmo::OPERATION::TRANSLATE);

}

void EditorLayer::OnDetach()
{
	
}

void EditorLayer::OnUpdate(Timestep deltaTime)
{
	m_CameraController->OnUpdate(deltaTime);
}

void EditorLayer::OnEvent(Event& event)
{
	m_CameraController->OnEvent(event);
}
