#pragma once

#include<glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>


#include "BufferManager.hpp"
#include "Velocity/ECS/Components.hpp"

namespace Velocity
{
	// Takes in a equirectangular HDRI image file and con-volutes into cubemaps (enviroment map, irradiance map, prefilter) and a brdfLUT
	class IBLMap
	{
		friend class Renderer;
	public:
		IBLMap(const std::string& filepath, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex, BufferManager& modelBuffer);

		~IBLMap();
	private:
		// Store references
		vk::UniqueDevice* r_Device;
		vk::PhysicalDevice r_PhysicalDevice;
		vk::CommandPool* r_CommandPool;
		uint32_t r_GraphicsQueueIndex;

		// Store the sampler as it will be used for all processes
		vk::UniqueSampler m_Sampler;

		// ENVIRONMENT MAP (Radiance)
		vk::Image			m_EnviromentMapImage;
		vk::DeviceMemory	m_EnviromentMapMemory;
		vk::ImageView		m_EnviromentMapImageView;

		vk::DescriptorImageInfo m_EnviromentMapImageInfo;
		vk::WriteDescriptorSet	m_EnviromentMapWriteSet;

		// TEMPORARY
		const glm::mat4 m_SkyboxMatrix = scale(glm::mat4(1.0f), glm::vec3(1000.0f, 1000.0f, 1000.0f));
		MeshComponent m_SphereMesh;
		
		// IRRADIANCE MAP

		// PREFILTER MAP

		// BRDF LUT

		// Preprocess methods - These will be run in constructor

		// Turns the flat image into a 6 layer image (like Skybox)
		void EquirectangularToCubemap(vk::UniqueImageView& equirectangularIV, BufferManager& modelBuffer);

		// Convolutes the cubemap to create an irradiance map
		void CreateIrradianceMap();

		// Prefilters the cubemap at multiple mip levels
		void PrefilterCubemap();

		// Calculates the 2D BRDF LUT for specular IBL
		void ComputeBRDF();
		
	};
}
