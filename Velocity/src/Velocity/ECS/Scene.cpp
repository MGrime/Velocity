#include "velpch.h"

#include "Scene.hpp"

#include <entt/entt.hpp>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include <snappy.h>

#include "Components.hpp"
#include "Entity.hpp"

#include <Velocity/Renderer/Renderer.hpp>

namespace Velocity
{
	Scene::Scene()
	{
		m_SceneName = "New Scene";
		m_Skybox = nullptr;
		
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

	void Scene::RemoveEntity(Entity& entity)
	{
		m_Registry.destroy(entity.m_EntityHandle);

		auto iterator = m_Entities.begin();
		for (auto it = m_Entities.begin(); it < m_Entities.end(); ++it)
		{
			if (it->m_EntityHandle == entity.m_EntityHandle)
			{
				m_Entities.erase(it);
				break;
			}
		}
	}
	void Scene::CreateSkybox(const std::string& baseFilepath, const std::string& extension)
	{
		m_Skybox = std::unique_ptr<Skybox>(Renderer::GetRenderer()->CreateSkybox(baseFilepath, extension));

	}
	
	void Scene::OnPointLightChanged(entt::registry& reg, entt::entity entity)
	{
		Renderer::GetRenderer()->UpdatePointlightArray();
	}


	Scene* Scene::LoadScene(const std::string& sceneFilepath)
	{
		Renderer::GetRenderer()->m_LogicalDevice->waitIdle();
		
		// Allocate memory
		auto* newScene = new Scene();

		// Allow for this to create a new scene
		if (!sceneFilepath.empty())
		{
			
			// Load registry from the file. Open in binary
			std::ifstream is;
			is.open(sceneFilepath, std::ios::binary | std::fstream::in);

			// Create string streams for compression processing
			std::string isUncompressed;

			// Read in file
			// go to end of file
			is.seekg(0, std::ios::end);
			// Allocate
			size_t size = is.tellg();
			std::string isCompressed(size,' ');
			// Go back to start
			is.seekg(0);
			is.read(&isCompressed[0], size);
		
			// Uncompress
			
			snappy::Uncompress(isCompressed.data(), isCompressed.size(), &isUncompressed);
			
			std::stringstream uncompressed{ isUncompressed };
			
			cereal::BinaryInputArchive archive(uncompressed);

			archive(newScene->m_SceneName);

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

				// Create a pointer and back with memory
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

			// Read in PBR Materials
			archive(renderer->m_PBRMaterials);

			// Check for skybox marker
			bool bHasSkybox = false;
			archive(bHasSkybox);

			if (bHasSkybox)
			{
				// Read in width/height
				uint32_t width, height;
				archive(width);
				archive(height);

				// read in pixels
				std::vector<stbi_uc> rawPixels;
				archive(rawPixels);

				// split into 6 layers
				std::array<std::unique_ptr<stbi_uc>,6> layerPointers;
				rawOffset = 0;
				for (size_t i = 0; i < 6; ++i)
				{
					const VkDeviceSize layerSize = width * height * 4;
					
					void* backedPixels = new stbi_uc[layerSize];
					memcpy(backedPixels, &rawPixels[rawOffset], layerSize);

					std::unique_ptr<stbi_uc> wrappedLayer = std::unique_ptr<stbi_uc>(static_cast<stbi_uc*>(backedPixels));
					layerPointers.at(i) = std::move(wrappedLayer);

					rawOffset += layerSize;
					
				}

				newScene->m_Skybox = std::unique_ptr<Skybox>(Renderer::GetRenderer()->CreateSkybox(layerPointers, width, height));
				
			}
		}
		else
		{
			auto& renderer = Renderer::GetRenderer();
			renderer->ClearState();

			// Load the skybox default
			Renderer::GetRenderer()->LoadMesh("../Velocity/assets/models/sphere.obj", "VEL_INTERNAL_Skybox");
			renderer->m_BufferManager->Sync();
			
			// Camera needs to be init at default
			newScene->m_SceneCamera = std::make_unique<Camera>();

			
		}

		return newScene;
	}

	void Scene::SaveScene(const std::string& saveFilepath)
	{
		// Open registry file
		std::ofstream finalOutput;
		finalOutput.open(saveFilepath,std::fstream::out | std::ios::binary);
		
		if (finalOutput.is_open())
		{
			// Create a string stream for the uncompressed data
			std::stringstream os;
			
			m_SceneName = GetRefName(saveFilepath);
			
			cereal::BinaryOutputArchive archive(os);

			archive(m_SceneName);

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

			// Archive materials list
			archive(renderer->m_PBRMaterials);

			// Archive skybox if exists
			bool bHasSkybox = false;
			if (m_Skybox)
			{
				// Archive a boolean
				bHasSkybox = true;
			}
			archive(bHasSkybox);
			if (bHasSkybox)
			{
				// Archive the actual skybox because the marker for it is in the scene file

				// Archive the width / height
				archive(m_Skybox->m_Width);
				archive(m_Skybox->m_Height);

				// Archive the pixels
				std::vector<stbi_uc> rawPixels;
				for (size_t i = 0; i < 6; ++i)
				{
					const VkDeviceSize layerSize = m_Skybox->m_Width * m_Skybox->m_Height * 4;
					for (size_t j = 0; j < layerSize; ++j)
					{
						// Traverse the pointer
						rawPixels.push_back(m_Skybox->m_RawPixels.at(i).get()[j]);
					}
				}

				archive(rawPixels);
			}

			// Now os contains the uncompressed string stream

			// Prepare a stream to store compressed data
			std::string osCompressed;

			// Compress
			snappy::Compress(os.str().data(), os.str().size(), &osCompressed);

			// Output to file
			finalOutput << osCompressed;

			// Close
			finalOutput.close();
			
		}
		else
		{
			VEL_CORE_ERROR("Failed to open file to save scene!");
		}
		
	}


}
