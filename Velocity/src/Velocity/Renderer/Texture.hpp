#pragma once

#include <string>

#include <vulkan/vulkan.hpp>

namespace Velocity
{
	// Holds an image for use in a shader
	// Also contains static helper functions
	
	class Texture
	{
		// Renderer needs to vulkan specific stuff behind the scenes but i dont want the user to see it
		friend class Renderer;
	public:
		// Class funcs
		// creates a texture from a file on the pc
		Texture(const std::string& filepath, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex);

		uint32_t GetWidth() { return m_Width; }
		uint32_t GetHeight() { return m_Height; }
		const std::string& GetPath() { return m_DebugPath; }

	
	private:
		// STATIC HELPERS
		// Can be called to transition an image
		static void TransitionImageLayout(vk::CommandBuffer& buffer, Texture& texture, vk::ImageLayout newLayout);
		static void CopyBufferToImage(vk::CommandBuffer& cmdBuffer, Texture& texture, vk::Buffer& buffer, uint32_t width, uint32_t height);
		static void GenerateMipmaps(vk::CommandBuffer& cmdBuffer, Texture& texture);
		
		// Member function proxy to allow texture->transition..
		void TransitionImageLayout(vk::CommandBuffer& buffer, vk::ImageLayout newLayout)
		{
			TransitionImageLayout(buffer, *this, newLayout);
		}

		void CopyBufferToImage(vk::CommandBuffer& cmdBuffer, vk::Buffer& buffer, uint32_t width, uint32_t height)
		{
			CopyBufferToImage(cmdBuffer, *this, buffer, width, height);
		}

		void GenerateMipmaps(vk::CommandBuffer& cmdBuffer)
		{
			GenerateMipmaps(cmdBuffer, *this);
		}
		
		// Convert stbi channels into vulkan image format
		static vk::Format stbiToVulkan(int channels);
		
		// Im not abstracting this as Texture IS the abstraction
		vk::UniqueImage				m_Image;
		vk::UniqueDeviceMemory		m_ImageMemory;

		// Tells the GPU how to
		vk::UniqueImageView			m_ImageView;

		// Store amount of mips created
		uint32_t					m_MipLevels;

		// Tells the pipeline how to bind this texture
		vk::DescriptorImageInfo		m_DescriptorImageInfo;

		// Variables
		std::string		m_DebugPath;
		uint32_t		m_Width;
		uint32_t		m_Height;
		vk::Format		m_CurrentFormat;
		vk::ImageLayout m_CurrentLayout;
		
		// References
		vk::UniqueDevice* r_Device;
		vk::PhysicalDevice r_PhysicalDevice;
		uint32_t r_GraphicsQueueIndex;
		
	};
}