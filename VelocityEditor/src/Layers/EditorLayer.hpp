#pragma once

#include <Velocity.hpp>

class EditorLayer : public Velocity::Layer
{
public:
	EditorLayer() : Layer("Editor Layer"){}
	
	void OnGuiRender() override;
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Velocity::Timestep deltaTime) override;
	void OnEvent(Velocity::Event& event) override;
private:

	// Stores all the data on the visible scene
	std::unique_ptr<Velocity::Scene> m_Scene;

	std::unique_ptr<Velocity::Skybox> m_Skybox;

	// Use a default camera
	std::unique_ptr<Velocity::DefaultCameraController> m_CameraController;

	// Load a sound clip to test
	Velocity::AudioManager::SoundClip m_SoundClip;
};