#include "EditorLayer.hpp"

#include "../Panels/SceneViewPanel.hpp"

void EditorLayer::OnGuiRender()
{
	SceneViewPanel::Draw(m_Scene.get());
}

void EditorLayer::OnAttach()
{
	auto& renderer = Renderer::GetRenderer();

	m_Scene = std::make_unique<Scene>();

	// Prepare camera
	m_CameraController = std::make_unique<DefaultCameraController>();
	m_CameraController->GetCamera()->SetPosition({ 0.0f,3.0f,3.0f });
	m_Scene->SetCamera(m_CameraController->GetCamera().get());

	// Load Mesh
	renderer->LoadMesh("assets/models/room.obj","Room");

	// Load texture
	renderer->CreateTexture("assets/textures/room.png", "Room");

	// Create entity
	auto entity = m_Scene->CreateEntity("Room");
	
	entity.AddComponent<MeshComponent>(renderer->GetMesh("Room"));
	entity.AddComponent<TextureComponent>(renderer->GetTextureByReference("Room"));
	
}

void EditorLayer::OnDetach()
{
	
}

void EditorLayer::OnUpdate(Timestep deltaTime)
{
	m_CameraController->OnUpdate(deltaTime);

	Renderer::GetRenderer()->BeginScene(m_Scene.get());

	Renderer::GetRenderer()->EndScene();
}

void EditorLayer::OnEvent(Event& event)
{
	m_CameraController->OnEvent(event);
}
