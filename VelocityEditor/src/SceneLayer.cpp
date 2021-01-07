#include "SceneLayer.hpp"

void SceneLayer::OnAttach()
{
	r_Renderer = Velocity::Renderer::GetRenderer();
	
	// Prepare the "camera"
	m_View = lookAt(glm::vec3(0.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const float width = static_cast<float>(Velocity::Application::GetWindow()->GetWidth());
	const float height = static_cast<float>(Velocity::Application::GetWindow()->GetHeight());
	m_Projection = glm::perspective(glm::radians(80.0f), width / height, 0.1f, 100.0f);
	//m_Projection[1][1] *= -1.0f;

	// "Load a mesh"
	r_Renderer->LoadMesh(m_Verts, m_Indices, "Square");

	r_Renderer->LoadMesh("assets/models/room.obj", "Room");
	r_Renderer->CreateTexture("assets/textures/room.png", "Room");
	
	// Load textures
	m_Textures.at(0) = r_Renderer->CreateTexture("assets/textures/logo.png", "Logo");
	m_Textures.at(1) = r_Renderer->CreateTexture("assets/textures/bronze.png","Bronze");
	m_Textures.at(2) = r_Renderer->CreateTexture("assets/textures/gold.png","Gold");
	m_Textures.at(3) = r_Renderer->CreateTexture("assets/textures/marble.png","Marble");
	m_Textures.at(4) = r_Renderer->CreateTexture("assets/textures/tiles.png","Tiles");

	// Setup matrix
	m_ModelMatrix = scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
	
	// Add more meshes
	std::srand(std::time(nullptr));
	for (int i = -5; i < 5; ++i)
	{
		// Move by a unit left in X
		float iF = static_cast<float>(i) * 1.1f;
		
		glm::mat4 newPos = translate(m_ModelMatrix, glm::vec3(iF, 0.0f, 0.0f));

		// Pick a random texture
		size_t indexRand = static_cast<size_t>(std::rand() % 4 + 1);
		
		r_Renderer->AddStatic(r_Renderer->GetMesh("Square"), m_Textures.at(indexRand), newPos);
	}
	
}

void SceneLayer::OnUpdate()
{
	// Dont update when paused. TODO TEMPORARY
	if (Velocity::Application::GetWindow()->IsPaused())
	{
		return;
	}

	Velocity::Renderer::GetRenderer()->BeginScene(m_View, m_Projection);

	// Nothing dynmaic yet
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	auto transform = translate(glm::mat4(1.0f),glm::vec3(0.0f,1.0f,1.0f)) * rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	for (int i = -5; i < 5; ++i)
	{
		// Move by a unit left in X
		float iF = static_cast<float>(i) * 3.0f;

		glm::mat4 newPos = translate(transform, glm::vec3(iF, 1.0f * static_cast<float>(i),0.0f));

		// Pick a random texture
		size_t indexRand = static_cast<size_t>(std::rand() % 4 + 1);

		r_Renderer->DrawDynamic(r_Renderer->GetMesh("Room"), r_Renderer->GetTextureByReference("Room"), newPos);
	}

	Velocity::Renderer::GetRenderer()->EndScene();

}
void SceneLayer::OnGuiRender()
{
	ImGui::Begin("GUI from scene layer!");

	ImGui::Button("Click me!");
	
	ImGui::End();
}