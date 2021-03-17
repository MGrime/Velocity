#include "velpch.h"

#include "Skybox.hpp"

#include <stb_image.h>

#include "Velocity/Renderer/Renderer.hpp"
#include "Velocity/Renderer/Texture.hpp"

namespace Velocity
{
	// Assumes 6 images with name format "baseFilepath_front.extension" etc
	Skybox::Skybox(const std::string& basefolder, const std::string& extension, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex)
	{
		r_Device = &device;
		r_PhysicalDevice = pDevice;
		r_GraphicsQueueIndex = graphicsQueueIndex;
		r_CommandPool = pool;
		
		#pragma region LOAD IMAGE

		m_IsLoadedByStbi = true;
		
		stbi_uc* textures[6];
		int width, height, channels;	// SHOULD BE SAME FOR ALL IMAGES
		for (int i = 0; i < 6; ++i)
		{
			textures[i] = stbi_load(CalculateFile(basefolder, extension, i).c_str(), &width, &height, &channels,STBI_rgb_alpha);
			m_RawPixels[i] = std::unique_ptr<stbi_uc>(textures[i]);
		}

		m_Width = width;
		m_Height = width;

		Init();

		#pragma endregion
	}

	// For internal use
	Skybox::Skybox(std::array<std::unique_ptr<stbi_uc>, 6>& pixels, int width, int height, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex)
	{
		r_Device = &device;
		r_PhysicalDevice = pDevice;
		r_GraphicsQueueIndex = graphicsQueueIndex;
		r_CommandPool = pool;
		m_IsLoadedByStbi = false;
		
		size_t index = 0;
		// Take ownership
		for (auto& pointer : pixels)
		{
			m_RawPixels[index] = std::move(pointer);
			++index;
		}
		

		m_Width = width;
		m_Height = width;

		Init();
	}
	
	void Skybox::Init()
	{
		// Calculate sizes
		const VkDeviceSize imageSize = m_Width * m_Height * 4 * 6;
		const VkDeviceSize layerSize = imageSize / 6;

		// Create staging buffer
		std::unique_ptr<BaseBuffer> stagingBuffer = std::make_unique<BaseBuffer>(
			r_PhysicalDevice,
			*r_Device,
			imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			);

		// Map
		void* data;
		auto result = r_Device->get().mapMemory(stagingBuffer->Memory.get(), 0, imageSize, vk::MemoryMapFlags{}, &data);
		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ERROR("Failed to load skybox: {0} (Failed to map memory)");
			VEL_CORE_ASSERT(false, "Failed to load skybox: {0} (Failed to map memory)");
			return;
		}

		for (uint8_t i = 0; i < 6; ++i)
		{
			// Offset into buffer by the memory size
			const auto offset = static_cast<char*>(data) + (layerSize * i);

			memcpy(static_cast<void*>(offset), m_RawPixels[i].get(), layerSize);

		}

		r_Device->get().unmapMemory(stagingBuffer->Memory.get());

#pragma endregion

#pragma region CREATE VULKAN IMAGE

		//vk::Format::eR32G32B32Sfloat;	// THIS IS THE FORMAT FOR HDRI

		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlagBits::eCubeCompatible,
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Srgb,
			vk::Extent3D{static_cast<uint32_t>(m_Width),static_cast<uint32_t>(m_Height),1},
			1,
			6,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			{},
			{},
			vk::ImageLayout::eUndefined
		};

		try
		{
			m_Image = r_Device->get().createImage(imageInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load skybox: (Failed to create image) Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to load skybox: (Failed to create image) Error: {0}", e.what());
			return;
		}

#pragma endregion

#pragma region ALLOCATE MEMORY

		vk::MemoryRequirements memRequirements = r_Device->get().getImageMemoryRequirements(m_Image);

		vk::MemoryAllocateInfo allocInfo = {
			memRequirements.size,
			BaseBuffer::FindMemoryType(r_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		try
		{
			m_ImageMemory = r_Device->get().allocateMemory(allocInfo);

		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load texture file: (Failed to create memory) Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to load texture file: (Failed to create image memory) Error: {0}",  e.what());
			return;
		}

#pragma endregion

		// Bind the image memory
		r_Device->get().bindImageMemory(m_Image, m_ImageMemory, 0);

#pragma region PROCESS RAW TO VULKAN
		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer processBufferWrapper = TemporaryCommandBuffer(*r_Device, r_CommandPool, queue);
			auto& processBuffer = processBufferWrapper.GetBuffer();

			// Set this image to destination optimal
			Texture::TransitionImageLayout(processBuffer, m_Image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 6);

			// Copy buffer
			Texture::CopyBufferToImage(processBuffer, m_Image, stagingBuffer->Buffer.get(), m_Width, m_Height, 6);

			// Set back to shader sampler
			Texture::TransitionImageLayout(processBuffer, m_Image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 6);
		}


#pragma endregion

#pragma region CREATE IMAGE VIEW
		vk::ImageViewCreateInfo viewInfo = {
			vk::ImageViewCreateFlags{},
			m_Image,
			vk::ImageViewType::eCube,
			vk::Format::eR8G8B8A8Srgb,
			{},
			{
				vk::ImageAspectFlagBits::eColor,
				0,1,0,6
			}
		};

		try
		{
			m_ImageView = r_Device->get().createImageView(viewInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load skybox:(Failed to create image view) Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to load skybox: (Failed to create image view) Error: {0}", e.what());
			return;
		}


#pragma endregion

#pragma region CREATE SAMPLER

		vk::SamplerCreateInfo sampler = {
			vk::SamplerCreateFlags{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge,
			0.0f,
			VK_TRUE,
			1.0f,
			VK_FALSE,
			vk::CompareOp::eNever,
			0.0f,
			1.0f,
			vk::BorderColor::eFloatOpaqueWhite,
			VK_FALSE
		};

		try
		{
			m_Sampler = r_Device->get().createSampler(sampler);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load skybox: (Failed to create sampler) Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to load skybox: (Failed to create sampler) Error: {0}",  e.what());
			return;
		}

		m_ImageInfo = {
			m_Sampler,
			m_ImageView,
			vk::ImageLayout::eShaderReadOnlyOptimal
		};


		m_WriteSet = {
			nullptr,
			1,
			0,
			1,
			vk::DescriptorType::eCombinedImageSampler,
			&m_ImageInfo,
			nullptr,
			nullptr
		};

		auto& loadedMeshes = Renderer::GetRenderer()->GetMeshList();

		// Dont load it twice
		if (loadedMeshes.find("VEL_INTERNAL_Skybox") != loadedMeshes.end())
		{
			Renderer::GetRenderer()->LoadMesh("../Velocity/assets/models/sphere.obj", "VEL_INTERNAL_Skybox");
		}

		m_SphereMesh = MeshComponent{ "VEL_INTERNAL_Skybox" };

#pragma endregion

	}
	
	Skybox::~Skybox()
	{
		r_Device->get().destroyImageView(m_ImageView);
		r_Device->get().destroyImage(m_Image);
		r_Device->get().destroySampler(m_Sampler);
		r_Device->get().freeMemory(m_ImageMemory);

		if (m_IsLoadedByStbi)
		{
			for (auto& pixels : m_RawPixels)
			{
				stbi_image_free(pixels.get());
				pixels.release();
			}

		}
	}

	
	std::string Skybox::CalculateFile(const std::string& base, const std::string& extension, int count)
	{
		std::string output;
		switch(count)
		{
		case 0:
			output = base + "/ft" + extension;
			break;
		case 1:
			output = base + "/bk" + extension;
			break;
		case 2:
			output = base + "/up" + extension;
			break;
		case 3:
			output = base + "/dn" + extension;
			break;
		case 4:
			output = base + "/rt" + extension;
			break;
		case 5:
			output = base + "/lf" + extension;
			break;
		default:
			output = "";
			break;
		}

		return output;
	}
}
