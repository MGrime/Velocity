#include "GameLayer.hpp"

void GameLayer::OnAttach()
{
	// Load assets

	// Load and set scene
	Scene* gameSceneRaw = Scene::LoadScene("assets/scenes/SIGame.velocity");
	m_GameScene.reset(gameSceneRaw);

	Renderer::GetRenderer()->SetScene(m_GameScene.get());

	// Extract camera
	m_Camera = std::make_unique<DefaultCameraController>(nullptr);
	m_Camera->SetCamera(m_GameScene->GetCamera());

	
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Timestep deltaTime)
{
	m_Camera->OnUpdate(deltaTime);
}

void GameLayer::OnGuiRender()
{
}

void GameLayer::OnEvent(Event& event)
{
}
