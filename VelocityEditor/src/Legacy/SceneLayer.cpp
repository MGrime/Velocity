//#include "SceneLayer.hpp"
//
//void SceneLayer::OnAttach()
//{
//	r_Renderer = Velocity::Renderer::GetRenderer();
//	
//	// Prepare the "camera"
//	m_CameraController = std::make_unique<Velocity::DefaultCameraController>();
//	m_CameraController->GetCamera()->SetPosition({ 0.0f,3.0f,3.0f });
//	m_CameraController->GetCamera()->SetRotation({ 0.0f,0.0f,0.0f });
//
//	m_CameraController->GetCamera()->GetViewMatrix();
//
//	// "Load a mesh"
//	r_Renderer->LoadMesh(m_Verts, m_Indices, "Square");
//
//	r_Renderer->LoadMesh("assets/models/room.obj", "Room");
//	r_Renderer->CreateTexture("assets/textures/room.png", "Room");
//	
//	// Load textures
//	m_Textures.at(0) = r_Renderer->CreateTexture("assets/textures/logo.png", "Logo");
//	m_Textures.at(1) = r_Renderer->CreateTexture("assets/textures/bronze.png","Bronze");
//	m_Textures.at(2) = r_Renderer->CreateTexture("assets/textures/gold.png","Gold");
//	m_Textures.at(3) = r_Renderer->CreateTexture("assets/textures/marble.png","Marble");
//	m_Textures.at(4) = r_Renderer->CreateTexture("assets/textures/tiles.png","Tiles");
//
//	m_Scene = std::make_unique<Velocity::Scene>();
//	m_Scene->SetCamera(m_CameraController->GetCamera().get());
//	
//
//	// Setup matrix
//	glm::mat4 scaleBase = scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
//	
//	// Add more meshes
//	std::srand(std::time(nullptr));
//	for (int i = -5; i < 5; ++i)
//	{
//		// Move by a unit left in X
//		float iF = static_cast<float>(i) * 1.1f;
//		
//		// Create an entity
//		m_StaticEntities.push_back(m_Scene->CreateEntity("Square" + std::to_string(i)));
//
//		// Get transform
//		auto& transform = m_StaticEntities.back().GetComponent<Velocity::TransformComponent>();
//		transform.Translation = glm::vec3(iF, 0.0f, 0.0f);
//		
//		// Pick a random texture
//		uint32_t indexRand = static_cast<uint32_t>(std::rand() % 4 + 1);
//		m_StaticEntities.back().AddComponent<Velocity::TextureComponent>(indexRand);
//
//		// Add a mesh
//		m_StaticEntities.back().AddComponent<Velocity::MeshComponent>(r_Renderer->GetMesh("Square"));	
//	}
//
//	for (int i = -5; i < 5; ++i)
//	{
//		// Move by a unit left in X
//		float iF = static_cast<float>(i) * 3.0f;
//
//		// Create an entity
//		m_DynamicEntities.push_back(m_Scene->CreateEntity("Room" + std::to_string(i)));
//
//		// Get transform
//		auto& transform = m_DynamicEntities.back().GetComponent<Velocity::TransformComponent>();
//		transform.Translation = glm::vec3(iF, 1.0f * static_cast<float>(i),0.0f);
//
//		// Pick a texture
//		m_DynamicEntities.back().AddComponent<Velocity::TextureComponent>(r_Renderer->GetTextureByReference("Room"));
//
//		// Add a mesh
//		m_DynamicEntities.back().AddComponent<Velocity::MeshComponent>(r_Renderer->GetMesh("Room"));
//	}
//	
//}
//
//void SceneLayer::OnUpdate(Velocity::Timestep deltaTime)
//{
//	// Dont update when paused. TODO TEMPORARY
//	if (Velocity::Application::GetWindow()->IsPaused())
//	{
//		return;
//	}
//
//	m_CameraController->OnUpdate(deltaTime);
//
//	Velocity::Renderer::GetRenderer()->SetScene(
//		m_Scene.get()
//	);
//
//	for (auto& entity : m_DynamicEntities)
//	{
//		auto& transform = entity.GetComponent<Velocity::TransformComponent>();
//		transform.Rotation = glm::vec3(0.0f, 0.0f, 90.0f * deltaTime);
//		
//	}
//
//	Velocity::Renderer::GetRenderer()->EndScene();
//
//}
//void SceneLayer::OnEvent(Velocity::Event& event)
//{
//	m_CameraController->OnEvent(event);
//}
//void SceneLayer::OnGuiRender()
//{
//	ImGui::Begin("GUI from scene layer!");
//
//	static bool show = true;
//	ImGui::ShowDemoWindow(&show);
//	
//	ImGui::End();
//}