#include "EditorLayer.hpp"


#include "../Panels/CameraStatePanel.hpp"
#include "../Panels/SceneViewPanel.hpp"
#include "../Panels/MainMenuPanel.hpp"
#include "Velocity/Utility/Input.hpp"

void EditorLayer::OnGuiRender()
{
	MainMenuPanel::Draw();
	SceneViewPanel::Draw(m_Scene.get());
	CameraStatePanel::Draw(*m_CameraController->GetCamera());
}

void EditorLayer::OnAttach()
{
	auto& renderer = Renderer::GetRenderer();

	m_Scene = std::make_unique<Scene>();

	// Prepare camera
	m_CameraController = std::make_unique<DefaultCameraController>();
	m_Scene->SetCamera(m_CameraController->GetCamera().get());

	// Load Mesh
	renderer->LoadMesh("assets/models/cube.obj","Cube");
	renderer->LoadMesh("assets/models/barrel.fbx", "Barrel");

	// Load texture
	renderer->CreateTexture("assets/materials/gold_albedo.png", "Gold");

	m_Skybox = std::unique_ptr<Skybox>(renderer->CreateSkybox("assets/textures/skyboxes/trance", ".jpg"));

	auto room = m_Scene->CreateEntity("Sphere");
	room.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 0.0f, 0.0f);
	room.GetComponent<TransformComponent>().Scale = glm::vec3(0.1f, 0.1f, 0.1f);
	room.AddComponent<MeshComponent>(Renderer::GetRenderer()->GetMesh("Barrel"));

	room.AddComponent<TextureComponent>(Renderer::GetRenderer()->GetTextureByReference("Gold"));
	//room.AddComponent<PBRComponent>(Renderer::GetRenderer()->CreatePBRMaterial("assets/materials/gold",".png","Gold"));

	m_Light = m_Scene->CreateEntity("Light");
	m_Light.RemoveComponent<TransformComponent>();
	m_Light.AddComponent<PointLightComponent>().Position = glm::vec3(4.0f, 4.0f, 4.0f);
	m_Light.AddComponent<MeshComponent>(Renderer::GetRenderer()->GetMesh("Cube"));

	m_Scene->SetSkybox(m_Skybox.get());
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

	Renderer::GetRenderer()->BeginScene(m_Scene.get());

	Renderer::GetRenderer()->EndScene();
}

void EditorLayer::OnEvent(Event& event)
{
	m_CameraController->OnEvent(event);
}
