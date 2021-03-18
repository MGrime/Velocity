#pragma once

#include <stb_image.h>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

#include "BaseBuffer.hpp"
#include <Velocity/ECS/Components.hpp>

namespace Velocity
{
	// Combines a cubemap texture, a sampler and the raw vertex data for an unindexed 1x1 cube
	class Skybox
	{
		friend class Renderer;
		
		// Scene needs to know about texture workings for serialisation
		friend class Scene;
	public:
		// Assumes 6 images with name format "baseFilepath_front.extension" etc
        Skybox(const std::string& basefolder, const std::string& extension, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex);

        ~Skybox();  
	private:
		Skybox(std::array<std::unique_ptr<stbi_uc>, 6>& pixels, int width, int height, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex);

		void Init();

		void GenerateMipmaps(vk::CommandBuffer& cmdBuffer);
		
		vk::Image			    m_Image;
		vk::ImageView		    m_ImageView;
		vk::DeviceMemory	    m_ImageMemory;
		uint32_t				m_MipLevels;

		vk::Sampler		        m_Sampler;

        vk::DescriptorImageInfo m_ImageInfo;
        vk::WriteDescriptorSet  m_WriteSet;

		MeshComponent			m_SphereMesh;

		// Storing for serilisation purposes
		uint32_t								m_Width;
		uint32_t								m_Height;
		std::array<std::unique_ptr<stbi_uc>,6>	m_RawPixels;
		bool m_IsLoadedByStbi;

        // References
        vk::UniqueDevice* r_Device;
        vk::PhysicalDevice r_PhysicalDevice;
		vk::CommandPool r_CommandPool;
        uint32_t r_GraphicsQueueIndex;

		// Helper functions
        std::string CalculateFile(const std::string& base, const std::string& extension, int count);

		const glm::mat4 m_SkyboxMatrix = scale(glm::mat4(1.0f), glm::vec3(1000.0f, 1000.0f, 1000.0f));

		
		
	};
}
