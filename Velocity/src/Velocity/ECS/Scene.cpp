#include "velpch.h"

#include "Scene.hpp"

#include <entt/entt.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Components.hpp"
#include "Entity.hpp"

#include <Velocity/Renderer/Renderer.hpp>

namespace Velocity
{
	Scene::Scene()
	{
		m_Registry.on_construct<PointLightComponent>().connect<&Scene::OnPointLightChanged>(this);
		m_Registry.on_destroy<PointLightComponent>().connect<&Scene::OnPointLightChanged>(this);
		m_Registry.on_update<PointLightComponent>().connect<&Scene::OnPointLightChanged>(this);
	}
	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "New Entity" : name;
		m_Entities.push_back(entity);
		return entity;
	}
	
	void Scene::OnPointLightChanged(entt::registry& reg, entt::entity entity)
	{
		Renderer::GetRenderer()->UpdatePointlightArray();
	}


	Scene* Scene::LoadScene(const std::string& sceneFilepath)
	{
		// Allocate memory
		auto* newScene = new Scene();

		// Allow for this to create a new scene
		if (!sceneFilepath.empty())
		{
			// Load registry from the file. Open in binary
			std::ifstream is;
			is.open(sceneFilepath + ".velreg", std::ios::binary | std::fstream::in);
			cereal::BinaryInputArchive archive(is);

			// Loads entity data from registry
			entt::snapshot_loader{ newScene->m_Registry }
				.entities(archive)
				.component<COMPONENT_LIST>(archive)
				.orphans();

			// Processes registry into m_Entities array
			newScene->m_Registry.each([&](auto rawEntity) {

				newScene->m_Entities.push_back({ rawEntity,newScene });

				});

			// Get renderer reference
			auto& renderer = Renderer::GetRenderer();

			// Clear the state before we load
			renderer->ClearState();

			// load the renderables list
			archive(renderer->m_Renderables);

			// load the raw state of the buffer manager
			renderer->m_BufferManager->Clear();
			archive(renderer->m_BufferManager->m_Vertices,renderer->m_BufferManager->m_Indices);
			renderer->m_BufferManager->Sync();

			// Process camera
			archive(newScene->m_SceneCamera);

			std::vector<std::string> flattenedTextureMapping;
			std::vector<stbi_uc> flattenedTextureRaw;
			
			std::vector<int> flattenedTextureSizes;
			
			// Read in flattened mappings
			archive(flattenedTextureMapping);
			archive(flattenedTextureRaw);
			archive(flattenedTextureSizes);

			// Loop in pairs at a time
			size_t rawOffset = 0;
			size_t sizeIndex = 0;
			for (size_t i = 0; i < flattenedTextureMapping.size(); ++i)
			{
				// Recalculate how much to go into the raw array with saved data
				VkDeviceSize texRawSize = flattenedTextureSizes[sizeIndex] * flattenedTextureSizes[sizeIndex + 1] * 4;

				// Create a unique pointer and back with memory
				void* backedPixels = new stbi_uc[texRawSize];
				// Copy into the pointer
				memcpy(backedPixels, &flattenedTextureRaw[rawOffset], texRawSize);
				// Wrap in a unique_ptr
				std::unique_ptr<stbi_uc> wrappedPixels = std::unique_ptr<stbi_uc>(static_cast<stbi_uc*>(backedPixels));
				// Move along offset
				rawOffset += texRawSize;

				// Recreate texture
				renderer->CreateTexture(std::move(wrappedPixels), flattenedTextureSizes[sizeIndex], flattenedTextureSizes[sizeIndex + 1], flattenedTextureMapping[i]);

				sizeIndex += 2;
			}
		}
		else
		{
			// Camera needs to be init at default
			newScene->m_SceneCamera = std::make_unique<Camera>();
		}

		return newScene;
	}

	void Scene::SaveScene(const std::string& saveFilepath)
	{
		// Open registry file
		std::ofstream os;
		os.open(saveFilepath + ".velreg",std::fstream::out | std::ios::binary);
		if (os.is_open())
		{
			cereal::BinaryOutputArchive archive(os);

			// Save registry
			entt::snapshot{ m_Registry }
				.entities(archive)
				.component<COMPONENT_LIST>(archive);

			// Get renderer reference
			auto& renderer = Renderer::GetRenderer();
			
			// Archive the renderables list
			archive(renderer->m_Renderables);

			// Archive the raw state of the buffer manager
			archive(renderer->m_BufferManager->m_Vertices, renderer->m_BufferManager->m_Indices);

			// Save out the camera details
			archive(m_SceneCamera);

			// Get list of textures
			std::vector<std::string> flattenedTextureMapping;
			std::vector<stbi_uc> flattenedTextureRaw;
			std::vector<int> flattenedTextureSizes;
			
			flattenedTextureMapping.reserve(renderer->m_Textures.size());
			// Cant reserve raw as size can drastically change
			flattenedTextureSizes.reserve(renderer->m_Textures.size() * 2);
			
			// Flatten the texture array
			for (size_t i = 1; i < renderer->m_Textures.size(); ++i)
			{
				auto& texture = renderer->m_Textures.at(i);
				
				flattenedTextureMapping.push_back(texture.first);

				flattenedTextureSizes.push_back(texture.second->m_Width);
				flattenedTextureSizes.push_back(texture.second->m_Height);

				// Loop raw pixels
				VkDeviceSize texRawSize = texture.second->m_Width * texture.second->m_Height * 4;
				for (size_t j = 0; j < texRawSize; ++j)
				{
					flattenedTextureRaw.push_back(texture.second->m_RawPixels.get()[j]);
				}
			}
			// Archive
			archive(flattenedTextureMapping);
			archive(flattenedTextureRaw);
			archive(flattenedTextureSizes);			
		}
		else
		{
			VEL_CORE_ERROR("Failed to open file to save scene!");
		}
		
	}


}
