#include "velpch.h"
#include "Texture.hpp"

#include <stb_image.h>

#include <Velocity/Core/Log.hpp>

#include <Velocity/Renderer/BaseBuffer.hpp>

namespace Velocity
{
	// creates a texture from a file on the pc
	Texture::Texture(const std::string& filepath, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex)
	{
		r_Device = &device;
		r_PhysicalDevice = pDevice;
		r_GraphicsQueueIndex = graphicsQueueIndex;

		#pragma region LOAD IMAGE

		int width, height, channels;
		stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		// 4 bytes per pixel for 32 bit images
		VkDeviceSize imageSize = width * height * 4;	
		
		if (!pixels)
		{
			VEL_CORE_ASSERT(false,"Failed to load texture image!");
			VEL_CORE_ERROR("Failed to load texture image!");
			return;
		}

		m_DebugPath = filepath;
		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);

		// Create staging buffer
		std::unique_ptr<BaseBuffer> stagingBuffer = std::make_unique<BaseBuffer>(
			r_PhysicalDevice,
			*r_Device,
			imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		void* data;
		auto result = r_Device->get().mapMemory(stagingBuffer->Memory.get(), 0, imageSize, vk::MemoryMapFlags{}, &data);
		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to map memory)",filepath);
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to map memory)", filepath);
			return;
		}

		memcpy(data, pixels, static_cast<size_t>(imageSize));

		r_Device->get().unmapMemory(stagingBuffer->Memory.get());

		// Free stbi data
		stbi_image_free(pixels);
		
		#pragma endregion

		#pragma region CREATE VULKAN IMAGE

		m_CurrentFormat = vk::Format::eR8G8B8A8Srgb;
		m_CurrentLayout = vk::ImageLayout::eUndefined;

		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			m_CurrentFormat,
			vk::Extent3D{m_Width,m_Height,1},
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			1,
			&r_GraphicsQueueIndex,
			m_CurrentLayout
		};

		// TODO: THIS MIGHT BE BREAKING IT IF IT DOESNT WORK. NOT SURE IF NEEDS QUEUE HERE ^^^^^^^^^^^^^

		try
		{
			m_Image = r_Device->get().createImageUnique(imageInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to create image) Error: {1}", filepath,e.what());
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to create image) Error: {1}", filepath,e.what());
			return;
		}
		
		#pragma endregion
		
		#pragma region ALLOCATE MEMORY

		vk::MemoryRequirements memRequirements = r_Device->get().getImageMemoryRequirements(m_Image.get());

		vk::MemoryAllocateInfo allocInfo = {
			memRequirements.size,
			BaseBuffer::FindMemoryType(r_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		try
		{
			m_ImageMemory = r_Device->get().allocateMemoryUnique(allocInfo);

		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to create image memory) Error: {1}", filepath, e.what());
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to create memory) Error: {1}", filepath, e.what());
			return;
		}

		#pragma endregion

		// Bind the image memory
		r_Device->get().bindImageMemory(m_Image.get(), m_ImageMemory.get(),0);

		#pragma region PROCESS RAW TO VULKAN

		{
			vk::Queue queue = r_Device->get().getQueue(graphicsQueueIndex, 0);
			TemporaryCommandBuffer processBufferWrapper = TemporaryCommandBuffer(*r_Device, pool, queue);
			auto& processBuffer = processBufferWrapper.GetBuffer();

			// Set this image to destination optimal
			TransitionImageLayout(processBuffer, vk::ImageLayout::eTransferDstOptimal);

			// Copy buffer
			CopyBufferToImage(processBuffer, stagingBuffer->Buffer.get(), width, height);

			// Transition to shader read
			TransitionImageLayout(processBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
			
		}
		
		#pragma endregion

		#pragma region CREATE IMAGE VIEW

		vk::ImageViewCreateInfo viewInfo = {
			vk::ImageViewCreateFlags{},
			m_Image.get(),
			vk::ImageViewType::e2D,
			m_CurrentFormat,
			{
			},
			{
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1
			}
		};
		try
		{
			m_ImageView = r_Device->get().createImageViewUnique(viewInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to create image view) Error: {1}", filepath, e.what());
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to create image view) Error: {1}", filepath, e.what());
			return;
		}
		
		#pragma endregion
	}

	void Texture::TransitionImageLayout(vk::CommandBuffer& buffer, Texture& texture, vk::ImageLayout newLayout)
	{
		vk::ImageMemoryBarrier barrier = {
			vk::AccessFlags{},
			vk::AccessFlags{},
			texture.m_CurrentLayout,
			newLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			texture.m_Image.get(),
			{
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1
			}
		};

		vk::PipelineStageFlags sourceStage{};
		vk::PipelineStageFlags destStage{};
		
		// Prepare missing parts based on data
		if (texture.m_CurrentLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destStage = vk::PipelineStageFlagBits::eTransfer;
			
		}
		else if (texture.m_CurrentLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			VEL_CORE_ASSERT(false, "Unsupported layout transition!");
			VEL_CORE_ERROR("Unsupported layout transition!");
		}
		

		buffer.pipelineBarrier(
			sourceStage,
			destStage,
			vk::DependencyFlags{},
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// Technically not true at this point but will be by time it matters
		texture.m_CurrentLayout = newLayout;
	}
	
	void Texture::CopyBufferToImage(vk::CommandBuffer& cmdBuffer, Texture& texture, vk::Buffer& buffer, uint32_t width, uint32_t height)
	{
		vk::BufferImageCopy region = {
			0,
			0,
			0,
			{
				vk::ImageAspectFlagBits::eColor,
				0,
				0,
				1
			},
			{
				0,0,0
			},
			{
				width,
				height,
				1
			}

		};

		cmdBuffer.copyBufferToImage(buffer, texture.m_Image.get(), vk::ImageLayout::eTransferDstOptimal, 1, &region);
	}

	// Convert stbi channels into vulkan image format
	vk::Format Texture::stbiToVulkan(int channels)
	{
		switch(channels)
		{
		case 1:
			return vk::Format::eR8Srgb;
		case 2:
			return vk::Format::eR8G8Srgb;
		case 3:
			return vk::Format::eR8G8B8Srgb;
		default:
			return vk::Format::eR8G8B8A8Srgb;
		}
	}
}
