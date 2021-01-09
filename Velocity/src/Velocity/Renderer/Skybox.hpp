#pragma once

#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

#include "BaseBuffer.hpp"

namespace Velocity
{
	// Combines a cubemap texture, a sampler and the raw vertex data for an unindexed 1x1 cube
	class Skybox
	{
		friend class Renderer;
	public:
		// Assumes 6 images with name format "baseFilepath_front.extension" etc
        Skybox(const std::string& baseFilepath, const std::string& extension, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex);

        ~Skybox();  
	private:
		vk::Image			    m_Image;
		vk::ImageView		    m_ImageView;
		vk::DeviceMemory	    m_ImageMemory;

		vk::Sampler		        m_Sampler;

        vk::DescriptorImageInfo m_ImageInfo;
        vk::WriteDescriptorSet  m_WriteSet;
		
		std::array<glm::vec3,36> m_Verts = {
            // positions          
           glm::vec3{-1.0f,  1.0f, -1.0f},
           glm::vec3{-1.0f, -1.0f, -1.0f},
           glm::vec3{ 1.0f, -1.0f, -1.0f},
           glm::vec3{ 1.0f, -1.0f, -1.0f},
           glm::vec3{ 1.0f,  1.0f, -1.0f},
           glm::vec3{-1.0f,  1.0f, -1.0f},

           glm::vec3{-1.0f, -1.0f,  1.0f},
           glm::vec3{-1.0f, -1.0f, -1.0f},
           glm::vec3{-1.0f,  1.0f, -1.0f},
           glm::vec3{-1.0f,  1.0f, -1.0f},
           glm::vec3{-1.0f,  1.0f,  1.0f},
           glm::vec3{-1.0f, -1.0f,  1.0f},

           glm::vec3{ 1.0f, -1.0f, -1.0f},
           glm::vec3{ 1.0f, -1.0f,  1.0f},
           glm::vec3{ 1.0f,  1.0f,  1.0f},
           glm::vec3{ 1.0f,  1.0f,  1.0f},
           glm::vec3{ 1.0f,  1.0f, -1.0f},
           glm::vec3{ 1.0f, -1.0f, -1.0f},

           glm::vec3{-1.0f, -1.0f,  1.0f},
           glm::vec3{-1.0f,  1.0f,  1.0f},
           glm::vec3{ 1.0f,  1.0f,  1.0f},
           glm::vec3{ 1.0f,  1.0f,  1.0f},
           glm::vec3{ 1.0f, -1.0f,  1.0f},
           glm::vec3{-1.0f, -1.0f,  1.0f},

           glm::vec3{-1.0f,  1.0f, -1.0f},
           glm::vec3{ 1.0f,  1.0f, -1.0f},
           glm::vec3{ 1.0f,  1.0f,  1.0f},
           glm::vec3{ 1.0f,  1.0f,  1.0f},
           glm::vec3{-1.0f,  1.0f,  1.0f},
           glm::vec3{-1.0f,  1.0f, -1.0f},

           glm::vec3{-1.0f, -1.0f, -1.0f},
           glm::vec3{-1.0f, -1.0f,  1.0f},
           glm::vec3{ 1.0f, -1.0f, -1.0f},
           glm::vec3{ 1.0f, -1.0f, -1.0f},
           glm::vec3{-1.0f, -1.0f,  1.0f},
           glm::vec3{ 1.0f, -1.0f,  1.0f},
        };

        std::unique_ptr<BaseBuffer> m_VertexBuffer;

        // References
        vk::UniqueDevice* r_Device;
        vk::PhysicalDevice r_PhysicalDevice;
        uint32_t r_GraphicsQueueIndex;

		// Helper functions
        std::string CalculateFile(const std::string& base, const std::string& extension, int count);

		
		
	};
}
