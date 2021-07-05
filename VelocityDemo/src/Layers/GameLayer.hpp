#pragma once

#include <Velocity.hpp>

using namespace Velocity;

class GameLayer final : public Layer
{
public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Timestep deltaTime) override;
	void OnGuiRender() override;
	void OnEvent(Event& event) override;
private:
	std::unique_ptr<Scene> m_GameScene;

	std::unique_ptr<DefaultCameraController> m_Camera;
};