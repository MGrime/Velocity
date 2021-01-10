#include "EditorLayer.hpp"

#include "../Panels/SceneViewPanel.hpp"

void EditorLayer::OnGuiRender()
{
	//SceneViewPanel::Draw(m_Scene.get());
}

void EditorLayer::OnAttach()
{
	auto& renderer = Renderer::GetRenderer();

	m_Scene = std::make_unique<Scene>();

	// Prepare camera
	m_CameraController = std::make_unique<DefaultCameraController>();
	m_CameraController->GetCamera()->SetPosition({ 0.0f,5.0f,0.0f });
	m_CameraController->GetCamera()->SetRotation({ -90.0f,0.0f,0.0f });
	m_Scene->SetCamera(m_CameraController->GetCamera().get());

	// Load Mesh
	renderer->LoadMesh("assets/models/pbrsphere.obj","Sphere");

	// Load texture
	renderer->CreateTexture("assets/textures/wood.jpg", "Wood");

	m_Skybox = std::unique_ptr<Skybox>(renderer->CreateSkybox("assets/textures/skyboxes/Park2", ".jpg"));

	auto room = m_Scene->CreateEntity("Sphere");
	room.GetComponent<TransformComponent>().Rotation.x = 90.0f;
	room.AddComponent<MeshComponent>(Renderer::GetRenderer()->GetMesh("Sphere"));
	room.AddComponent<TextureComponent>(TextureComponent{ Renderer::GetRenderer()->GetTextureByReference("Wood") });

	m_Scene->SetSkybox(m_Skybox.get());
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
