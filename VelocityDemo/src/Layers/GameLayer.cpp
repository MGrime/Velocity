#include "GameLayer.hpp"

void GameLayer::OnAttach()
{
	m_TestSound = AudioManager::GetManager()->LoadSound("assets/sounds/test.wav");
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Timestep deltaTime)
{
	m_TestSound.Play();
}

void GameLayer::OnGuiRender()
{
}

void GameLayer::OnEvent(Event& event)
{
}
