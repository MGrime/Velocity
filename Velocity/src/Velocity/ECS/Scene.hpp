#pragma once
#include <entt/entt.hpp>
#include <Velocity/Utility/Camera.hpp>

#include <Velocity/Renderer/Skybox.hpp>

#include "Velocity/Renderer/IBLMap.hpp"

namespace Velocity
{
	class Entity;
	
	class Scene
	{
	public:
		Scene();

		Scene(const std::string& sceneFilepath);
		
		~Scene() = default;

		Entity CreateEntity(const std::string & name = "New Entity");

		void SetSkybox(Skybox* skybox) { m_Skybox = skybox; }

		std::vector<Entity>& GetEntities() { return m_Entities; }

		Camera* GetCamera() const { return m_SceneCamera.get(); }

		// Static function to load a default scene from a file
		// Filepath should be relative path with no file extension
		// Blank string to create new
		static Scene* LoadScene(const std::string& sceneFilepath);

		// Saves out data from given scene
		void SaveScene(const std::string& saveFilepath);
	
	private:
		// Collection of entities we have made
		std::vector<Entity> m_Entities;
		
		// Collection of entt entiies
		entt::registry m_Registry;

		// Scene camera
		std::unique_ptr<Camera> m_SceneCamera = nullptr;

		// Scene skybox
		Skybox* m_Skybox = nullptr;

		void OnPointLightChanged(entt::registry& reg, entt::entity entity);
		
		// Both need access to registry but the end user doesnt
		friend class Renderer;
		friend class Entity;
	};
	
}
