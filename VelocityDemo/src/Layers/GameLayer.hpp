#pragma once

#include <Velocity.hpp>

using namespace Velocity;

struct MovementComponent
{
	float Life;
	float MoveSpeed;
};

struct EnemyComponent
{
	bool Alive = true;
	float MoveSpeed = 1.0f;
};

class GameLayer final : public Layer
{
	// Constants
	const float PLAYER_MOVE_SPEED = 10.0f;
	const float PLAYER_TILT_CAP = glm::radians(15.0f);

	const float LEFT_PLAY_BOUND = -25.0f;
	const float RIGHT_PLAY_BOUND = 25.0f;

	const float RIGHT_ENEMY_SPAWN = 18.0f;

	const float TOP_PLAY_BOUND = 24.0f;

public:
	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Timestep deltaTime) override;
	void OnGuiRender() override;
	void OnEvent(Event& event) override;
private:
	std::unique_ptr<Scene> m_GameScene;

	// Extracted meshes/mats/entites from the scene
	PBRComponent				m_EnemyShipMaterial;
	PBRComponent				m_LaserMaterial;
	Entity						m_PlayerEntityHandle;
	std::unique_ptr<DefaultCameraController> m_SceneCamera;
	
	// Loaded music and sounds
	AudioManager::SoundClip		m_BackgroundMusic;
	AudioManager::SoundClip		m_ThrustSound;
	AudioManager::SoundClip		m_LaserSound;
	AudioManager::SoundClip		m_ExplodeSound;
	
	// Create an new bullet entity at position
	Entity CreateBullet(glm::vec3 position);

	// Handle Player Movement
	void ProcessPlayerMovement(Timestep deltaTime);

	// Handle bullets
	void ProcessBullets(Timestep deltaTime);

	// Handle enemies
	void ProcessEnemies(Timestep deltaTime);

	bool m_IsGameOver = false;
	bool m_DidWin = true;

};