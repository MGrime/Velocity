#pragma once
#include <entt/entt.hpp>
#include <Velocity/Utility/Camera.hpp>

namespace Velocity
{
	class Entity;
	
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		Entity CreateEntity(const std::string & name = "New Entity");

		void SetCamera(Camera* camera) { m_Camera = camera; }
	
	private:
		// Collection of entt entiies
		entt::registry m_Registry;

		// Scene camera
		Camera* m_Camera;

		// Both need access to registry but the end user doesnt
		friend class Renderer;
		friend class Entity;
	};
	
}
