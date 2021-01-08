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

		std::vector<Entity>& GetEntities() { return m_Entities; }
	
	private:
		// Collection of entities we have made
		std::vector<Entity> m_Entities;
		
		// Collection of entt entiies
		entt::registry m_Registry;

		// Scene camera
		Camera* m_Camera;

		// Both need access to registry but the end user doesnt
		friend class Renderer;
		friend class Entity;
	};
	
}
