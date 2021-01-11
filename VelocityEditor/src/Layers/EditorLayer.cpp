#include "EditorLayer.hpp"

#include "../Panels/SceneViewPanel.hpp"

void EditorLayer::OnGuiRender()
{
	SceneViewPanel::Draw(m_Scene.get());

	ImGui::Begin("Camera State");

	auto pos = m_CameraController->GetCamera()->GetPosition();
	
	ImGui::Text("X: %f Y: %f Z: %f", pos.x, pos.y, pos.z);
	
	ImGui::End();
}

void EditorLayer::OnAttach()
{
	auto& renderer = Renderer::GetRenderer();

	m_Scene = std::make_unique<Scene>();

	// Prepare camera
	m_CameraController = std::make_unique<DefaultCameraController>();
	m_Scene->SetCamera(m_CameraController->GetCamera().get());

	// Load Mesh
	renderer->LoadMesh("assets/models/cube.obj","Sphere");

	// Load texture
	renderer->CreateTexture("assets/textures/wood.jpg", "Wood");

	m_Skybox = std::unique_ptr<Skybox>(renderer->CreateSkybox("assets/textures/skyboxes/Park2", ".jpg"));

	auto room = m_Scene->CreateEntity("Sphere");
	room.GetComponent<TransformComponent>().Translation = glm::vec3(0.0f, 0.0f, 0.0f);
	room.AddComponent<MeshComponent>(Renderer::GetRenderer()->GetMesh("Sphere"));
	room.AddComponent<TextureComponent>(TextureComponent{ Renderer::GetRenderer()->GetTextureByReference("Wood") });

	m_Light = m_Scene->CreateEntity("Light");
	m_Light.RemoveComponent<TransformComponent>();
	m_Light.AddComponent<PointLightComponent>().Position = glm::vec3(4.0f, 4.0f, 4.0f);
	m_Light.AddComponent<MeshComponent>(Renderer::GetRenderer()->GetMesh("Sphere"));

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
