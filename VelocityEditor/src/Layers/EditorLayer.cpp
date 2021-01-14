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
	m_CameraController->GetCamera()->SetPosition({ 9.16f,5.64f,-4.82 });
	m_CameraController->GetCamera()->SetRotation({ 21.05,-54.85 });
	
	// Load Mesh
	renderer->LoadMesh("assets/models/cube.obj","Cube");
	renderer->LoadMesh("assets/models/barrel.fbx", "Barrel");
	renderer->LoadMesh("assets/models/pbrsphere.obj", "Sphere");
	renderer->LoadMesh("assets/models/rocket.fbx", "Rocket");

	// Load texture
	renderer->CreateTexture("assets/materials/pirate-gold_albedo.png", "Gold");

	m_Skybox = std::unique_ptr<Skybox>(renderer->CreateSkybox("assets/textures/skyboxes/trance", ".jpg"));

	auto room = m_Scene->CreateEntity("Rocket");
	room.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 0.0f, 0.0f);
	room.GetComponent<TransformComponent>().Rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
	room.AddComponent<MeshComponent>("Sphere");

	//room.AddComponent<TextureComponent>(Renderer::GetRenderer()->GetTextureByReference("Gold"));
	room.AddComponent<PBRComponent>(Renderer::GetRenderer()->CreatePBRMaterial("assets/materials/pirate-gold",".png","Rocket",false));

	m_Light = m_Scene->CreateEntity("Light");
	m_Light.RemoveComponent<TransformComponent>();
	m_Light.AddComponent<PointLightComponent>().Position = glm::vec3(4.0f, 7.0f, 4.0f);
	m_Light.AddComponent<MeshComponent>("Cube");

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
