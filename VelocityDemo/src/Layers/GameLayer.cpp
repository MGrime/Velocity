#include "GameLayer.hpp"
void GameLayer::OnAttach()
{
	// Load assets

	// Load and set scene
	Scene* gameSceneRaw = Scene::LoadScene("assets/scenes/SIGame.velocity");
	m_GameScene.reset(gameSceneRaw);
	
	// Extract assets
	for (auto& entity : m_GameScene->GetEntities())
	{
		if (entity.GetComponent<TagComponent>().Tag == "Player")
		{
			m_PlayerEntityHandle = entity;
			break;
		}
	}

	// Scale down player
	m_PlayerEntityHandle.GetComponent<TransformComponent>().Scale *= glm::vec3(0.7f);

	// Extract assets
	m_EnemyShipMaterial = Renderer::GetRenderer()->GetMaterialsList().at("enemy-spaceship");
	m_LaserMaterial = Renderer::GetRenderer()->GetMaterialsList().at("laser");

	m_SceneCamera = std::make_unique<DefaultCameraController>(m_GameScene->GetCamera());

	Renderer::GetRenderer()->SetScene(m_GameScene.get());

	// Create enemies	
	for (int i = 0; i < 6; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			auto enemy = m_GameScene->CreateEntity("Enemy");
			enemy.AddComponent<MeshComponent>(MeshComponent{ "spaceship.obj" });
			enemy.AddComponent<PBRComponent>(m_EnemyShipMaterial);
			enemy.AddComponent<EnemyComponent>();	// Just here for loop later

			auto& transform = enemy.GetComponent<TransformComponent>();
			transform.Translation = glm::vec3(RIGHT_ENEMY_SPAWN - 7.0f * i, 0.0f, TOP_PLAY_BOUND - 5.0f * j);
			transform.Scale = glm::vec3(0.6f);
			transform.Rotation = glm::vec3(0.0f, glm::radians(180.0f), 0.0f);
		}
	}
	

	// Load and play background music
	m_BackgroundMusic = AudioManager::GetManager()->LoadSound("assets/music/background_music.wav");
	m_BackgroundMusic.SetLoop(true);
	m_BackgroundMusic.SetVolume(0.5f);
	m_BackgroundMusic.Play();

	// Load thruster music and loop/play
	m_ThrustSound = AudioManager::GetManager()->LoadSound("assets/music/thruster.wav");
	m_ThrustSound.SetLoop(true);
	m_ThrustSound.SetVolume(0.0f);
	m_ThrustSound.Play();

	m_LaserSound = AudioManager::GetManager()->LoadSound("assets/music/laser.wav");
	m_LaserSound.SetLoop(false);
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(Timestep deltaTime)
{
	if (!m_IsGameOver)
	{
		ProcessPlayerMovement(deltaTime);
		ProcessBullets(deltaTime);
		ProcessEnemies(deltaTime);
	}
}

void GameLayer::OnGuiRender()
{
	// Init flags
	const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	// Set state
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().ViewportImageSize.x * 0.5f, ImGui::GetIO().ViewportImageSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowBgAlpha(0.35f);

	if (ImGui::Begin("Game Overlay", nullptr, window_flags))
	{
		
	}
	ImGui::End();
}

void GameLayer::OnEvent(Event& event)
{
	m_SceneCamera->OnEvent(event);
}

Entity GameLayer::CreateBullet(glm::vec3 position)
{
	Entity newBullet = m_GameScene->CreateEntity("NewBullet");

	newBullet.AddComponent<MeshComponent>(MeshComponent{ "bullet.obj" });
	newBullet.AddComponent<PBRComponent>(m_LaserMaterial);
	newBullet.AddComponent<MovementComponent>(MovementComponent{ 2.0f,40.0f });

	newBullet.GetComponent<TransformComponent>().Translation = position;
	newBullet.GetComponent<TransformComponent>().Rotation = glm::vec3(0.0f, 0.0f, glm::radians(180.0f));
	newBullet.GetComponent<TransformComponent>().Scale = glm::vec3(0.2f, 0.2f, 0.2f);
	

	return newBullet;
}

void GameLayer::ProcessPlayerMovement(Timestep deltaTime)
{
#pragma region MOVEMENT
	auto& transform = m_PlayerEntityHandle.GetComponent<TransformComponent>();

	static float TIME_HELD = 0.0f;
	static float TIME_VOL_HELD = 0.0f;
	
	if (Input::IsKeyHeld(VEL_KEY_A))
	{
		transform.Translation -= glm::vec3(PLAYER_MOVE_SPEED * deltaTime, 0.0f, 0.0f);

		TIME_HELD += deltaTime;
		TIME_VOL_HELD += deltaTime;
	}
	else if (Input::IsKeyHeld(VEL_KEY_D))
	{
		transform.Translation += glm::vec3(PLAYER_MOVE_SPEED * deltaTime, 0.0f, 0.0f);
		
		TIME_HELD -= deltaTime;
		TIME_VOL_HELD += deltaTime;
	}
	else
	{
		if (TIME_HELD < 0.0f)
		{
			TIME_HELD += deltaTime;
		}
		else if (TIME_HELD >0.0f)
		{
			TIME_HELD -= deltaTime;
		}
		else
		{
			TIME_HELD = 0.0f;
		}

		TIME_VOL_HELD -= deltaTime;
		if (TIME_VOL_HELD < 0.0f)
		{
			TIME_VOL_HELD = 0.0f;
		}
	}
	// Bound check
	if (TIME_HELD < -1.0f) { TIME_HELD = -1.0f; }
	if (TIME_HELD > 1.0f) { TIME_HELD = 1.0f; }
	// Bound check
	if (TIME_VOL_HELD < 0.0f) { TIME_VOL_HELD = 0.0f; }
	if (TIME_VOL_HELD > 1.0f) { TIME_VOL_HELD = 1.0f; }

	// Tilt
	transform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f + PLAYER_TILT_CAP * TIME_HELD);

	// Use ABS of same counter for thrust sound
	m_ThrustSound.SetVolume(0.0f + (1.0f * TIME_VOL_HELD));

	// Enforce bounds
	if (transform.Translation.x <= LEFT_PLAY_BOUND)
	{
		transform.Translation.x = LEFT_PLAY_BOUND;
	}
	else if (transform.Translation.x >= RIGHT_PLAY_BOUND)
	{
		transform.Translation.x = RIGHT_PLAY_BOUND;
	}
#pragma endregion

#pragma region FIRING

	static float FIRE_RATE_LIMITER = 0.0f;
	
	if (Input::IsKeyPressed(VEL_KEY_SPACE) && FIRE_RATE_LIMITER <= 0.01f)
	{
		// Create bullet at my pos
		CreateBullet(transform.Translation);
		m_LaserSound.Play();

		FIRE_RATE_LIMITER = 0.3f;
	}
	if (FIRE_RATE_LIMITER > 0.0f)
	{
		FIRE_RATE_LIMITER -= deltaTime;
	}
	else
	{
		FIRE_RATE_LIMITER = 0.0f;
	}
	
#pragma endregion
}

// Handle bullets
void GameLayer::ProcessBullets(Timestep deltaTime)
{
	// Process bullets
	for (auto& entity : m_GameScene->GetEntities())
	{
		if (entity.HasComponent<MovementComponent>())
		{
			auto& movement = entity.GetComponent<MovementComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();

			movement.Life -= deltaTime;
			transform.Translation += glm::vec3(0.0f, 0.0f, movement.MoveSpeed * deltaTime);
		}
	}

	// Can do this as bullets being erased a frame late isnt end of world
	for (auto& entity : m_GameScene->GetEntities())
	{
		if (entity.HasComponent<MovementComponent>())
		{
			if (entity.GetComponent<MovementComponent>().Life <= 0.0f)
			{
				m_GameScene->RemoveEntity(entity);
				break;
			}
		}
	}
}

void GameLayer::ProcessEnemies(Timestep deltaTime)
{
	static enum
	{
		left,
		right
	} direction = left;

	bool hasCrossed = false;
	
	for (auto& entity : m_GameScene->GetEntities())
	{
		if (entity.HasComponent<EnemyComponent>())
		{
			auto& enemy = entity.GetComponent<EnemyComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			float moveAmountX;
			if (direction == left)
			{
				moveAmountX = -(enemy.MoveSpeed * deltaTime);
			}
			else
			{
				moveAmountX = enemy.MoveSpeed * deltaTime;
			}
			transform.Translation += glm::vec3(moveAmountX,0.0f,0.0f);

			if (direction == left)
			{
				if (transform.Translation.x <= LEFT_PLAY_BOUND)
				{
					hasCrossed = true;
				}
			}
			else
			{
				if (transform.Translation.x >= RIGHT_PLAY_BOUND)
				{
					hasCrossed = true;
				}
			}
		}
	}

	// If this is true the current "row" has reached the end so move in Y
	if (hasCrossed)
	{
		for (auto& entity : m_GameScene->GetEntities())
		{
			if (entity.HasComponent<EnemyComponent>())
			{
				entity.GetComponent<EnemyComponent>().MoveSpeed += 0.5f;
				auto& transform = entity.GetComponent<TransformComponent>();
				transform.Translation.z -= 2.5f;

				// Invaders have passed over player so loss
				if (const float offset = glm::abs(transform.Translation.z - m_PlayerEntityHandle.GetComponent<TransformComponent>().Translation.z); 
					offset <= 2.0f)
				{
					m_IsGameOver = true;
					m_DidWin = false;
				}
			}
		}

		if (direction == left)
		{
			direction = right;
		}
		else
		{
			direction = left;
		}
	}
}