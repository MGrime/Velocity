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
		~Scene() = default;

		Entity CreateEntity(const std::string & name = "New Entity");

		void CreateSkybox(const std::string& baseFilepath, const std::string& extension);
		void RemoveSkybox() { m_Skybox.reset(nullptr); }

		std::vector<Entity>& GetEntities() { return m_Entities; }
		const std::string& GetSceneName() { return m_SceneName; }

		Camera* GetCamera() const { return m_SceneCamera.get(); }
		Skybox* GetSkybox() const { if (m_Skybox) { return m_Skybox.get(); } return nullptr; }

		// Static function to load a default scene from a file
		// Filepath should be relative path with no file extension
		// Blank string to create new
		static Scene* LoadScene(const std::string& sceneFilepath);

		// Saves out data from given scene
		void SaveScene(const std::string& saveFilepath);

		// Extracts reference names from filepaths for scene management
		static std::string GetRefName(const std::string& fullPath)
		{
			std::string modifyPath = fullPath;
			// Find last
			std::string fileCharacter = "/";
#ifdef VEL_PLATFORM_WINDOWS
			fileCharacter = "\\";
#endif

			// Get just the file name
			auto lastSlashPos = fullPath.find_last_of(fileCharacter);
			// Should never miss this but checking just in case
			if (lastSlashPos != std::string::npos)
			{
				// Windows needs to erase one more
#ifdef VEL_PLATFORM_WINDOWS
				modifyPath.erase(0, lastSlashPos + 1);
#elif
				modifyPath.erase(0, lastSlashPos);
#endif
			}

			// Also need to erase .velocity
			auto velocityPos = modifyPath.find(".velocity");
			modifyPath.erase(velocityPos, modifyPath.length() - 1); 

			return modifyPath;
		}
	
	private:
		std::string m_SceneName;
		
		// Collection of entities we have made
		std::vector<Entity> m_Entities;
		
		// Collection of entt entiies
		entt::registry m_Registry;

		// Scene camera
		std::unique_ptr<Camera> m_SceneCamera = nullptr;

		// Scene skybox
		std::unique_ptr <Skybox> m_Skybox = nullptr;

		void OnPointLightChanged(entt::registry& reg, entt::entity entity);
		
		// Both need access to registry but the end user doesnt
		friend class Renderer;
		friend class Entity;

		
	};
	
}
