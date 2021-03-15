#include "velpch.h"
#include "Texture.hpp"

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
		r_Pool = pool;
		
		int width, height, channels;
		m_RawPixels = std::unique_ptr<stbi_uc>(
			stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha)
		);

		m_IsLoadedByStbi = true;

		m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max<int>(width, height)))) + 1;

		m_FilePath = filepath;
		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);

		Init();
		
	}

	// Private constructor
	Texture::Texture(std::unique_ptr<stbi_uc> pixels, int width, int height, vk::UniqueDevice& device,
		vk::PhysicalDevice& pDevice, vk::CommandPool& pool, uint32_t& graphicsQueueIndex)
	{
		r_Device = &device;
		r_PhysicalDevice = pDevice;
		r_GraphicsQueueIndex = graphicsQueueIndex;
		r_Pool = pool;

		m_RawPixels = std::move(pixels);

		m_IsLoadedByStbi = false;

		m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max<int>(width, height)))) + 1;

		// TODO: Check this 
		m_FilePath = "Raw from serialisation";
		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);

		Init();
	}

	void Texture::Init()
	{
		// 4 bytes per pixel for 32 bit images
		VkDeviceSize imageSize = m_Width * m_Height * 4;

		if (!m_RawPixels)
		{
			VEL_CORE_ERROR("Failed to load texture image!");
			VEL_CORE_ASSERT(false, "Failed to load texture image!");
			return;
		}

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
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to map memory)", filepath);
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to map memory)", m_FilePath);
			return;
		}

		memcpy(data, m_RawPixels.get(), static_cast<size_t>(imageSize));

		r_Device->get().unmapMemory(stagingBuffer->Memory.get());

#pragma endregion

#pragma region CREATE VULKAN IMAGE

		m_CurrentFormat = vk::Format::eR8G8B8A8Srgb;
		m_CurrentLayout = vk::ImageLayout::eUndefined;

		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			m_CurrentFormat,
			vk::Extent3D{m_Width,m_Height,1},
			m_MipLevels,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
			vk::SharingMode::eExclusive,
			{},
			{},
			m_CurrentLayout
		};

		// TODO: THIS MIGHT BE BREAKING IT IF IT DOESNT WORK. NOT SURE IF NEEDS QUEUE HERE ^^^^^^^^^^^^^

		try
		{
			m_Image = r_Device->get().createImageUnique(imageInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to create image) Error: {1}", m_FilePath, e.what());
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to create image) Error: {1}", filepath, e.what());
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
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to create memory) Error: {1}", m_FilePath, e.what());
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to create image memory) Error: {1}", filepath, e.what());
			return;
		}

#pragma endregion

		// Bind the image memory
		r_Device->get().bindImageMemory(m_Image.get(), m_ImageMemory.get(), 0);

#pragma region PROCESS RAW TO VULKAN

		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer processBufferWrapper = TemporaryCommandBuffer(*r_Device, r_Pool, queue);
			auto& processBuffer = processBufferWrapper.GetBuffer();

			// Set this image to destination optimal
			TransitionImageLayout(processBuffer, vk::ImageLayout::eTransferDstOptimal);

			// Copy buffer
			CopyBufferToImage(processBuffer, stagingBuffer->Buffer.get(), m_Width, m_Height);
		}

		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer processBufferWrapper = TemporaryCommandBuffer(*r_Device, r_Pool, queue);
			auto& processBuffer = processBufferWrapper.GetBuffer();
			// Mipmap (also transfers to shader layout)
			GenerateMipmaps(processBuffer);
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
				m_MipLevels,
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
			VEL_CORE_ERROR("Failed to load texture file: {0} (Failed to create image view) Error: {1}", m_FilePath, e.what());
			VEL_CORE_ASSERT(false, "Failed to load texture file: {0} (Failed to create image view) Error: {1}", filepath, e.what());
			return;
		}

#pragma endregion
	}

	void Texture::TransitionImageLayout(vk::CommandBuffer& buffer, vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t miplevels, uint32_t layerCount)
	{
		vk::ImageMemoryBarrier barrier = {
			vk::AccessFlags{},
			vk::AccessFlags{},
			oldLayout,
			newLayout,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			image,
			{
				vk::ImageAspectFlagBits::eColor,
				0,
				miplevels,
				0,
				layerCount
			}
		};

		vk::PipelineStageFlags sourceStage{};
		vk::PipelineStageFlags destStage{};
		
		// Prepare missing parts based on data
		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destStage = vk::PipelineStageFlagBits::eTransfer;
			
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
			destStage = vk::PipelineStageFlagBits::eTransfer;
			
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlags{};

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destStage = vk::PipelineStageFlagBits::eBottomOfPipe;
			
		}
		else
		{
			VEL_CORE_ERROR("Unsupported layout transition!");
			VEL_CORE_ASSERT(false, "Unsupported layout transition!");
		}
		

		buffer.pipelineBarrier(
			sourceStage,
			destStage,
			vk::DependencyFlags{},
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
	
	void Texture::CopyBufferToImage(vk::CommandBuffer& cmdBuffer, vk::Image image, vk::Buffer& buffer, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		vk::BufferImageCopy region = {
			0,
			0,
			0,
			{
				vk::ImageAspectFlagBits::eColor,
				0,
				0,
				layerCount
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

		cmdBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
	}

	void Texture::GenerateMipmaps(vk::CommandBuffer& cmdBuffer, Texture& texture)
	{
		// Check for compability
		vk::FormatProperties formatProperties = texture.r_PhysicalDevice.getFormatProperties(texture.m_CurrentFormat);

		if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
		{
			VEL_CORE_ERROR("Texture doesn't support mipmap generation!");
			VEL_CORE_ASSERT(false, "Texture doesn't support mipmap generation!");
			return;
		}
		
		// Set up a barrier. We will reuse it, so some things will not be changed
		vk::ImageMemoryBarrier barrier = {
			vk::AccessFlags{},
			vk::AccessFlags{},
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eUndefined,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			texture.m_Image.get(),
			vk::ImageSubresourceRange{
				vk::ImageAspectFlagBits::eColor,
				0,
				1,0,1
			}
		};

		int mipWidth = texture.GetWidth();
		int mipHeight = texture.GetHeight();
		
		for (uint32_t i = 1; i < texture.m_MipLevels; ++i)
		{
			// 1. Transition level i-1 to transfer src
			
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			cmdBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags{},
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			// 2. Blit from i-1 -> i with half size each time
			vk::ImageBlit blit{};

			blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
			blit.srcOffsets[1] = vk::Offset3D{ mipWidth,mipHeight, 1 };
			blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = vk::Offset3D{ 0,0,0 };
			blit.dstOffsets[1] = vk::Offset3D{
				mipWidth > 1 ? mipWidth / 2 : 1,
				mipHeight > 1 ? mipHeight / 2 : 1,
				1
			};
			blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			
			cmdBuffer.blitImage(
				texture.m_Image.get(),
				vk::ImageLayout::eTransferSrcOptimal,
				texture.m_Image.get(),
				vk::ImageLayout::eTransferDstOptimal,
				1,
				&blit,
				vk::Filter::eLinear
			);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			cmdBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags{},
				0,nullptr,
				0,nullptr,
				1,&barrier
			);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
			
		}

		barrier.subresourceRange.baseMipLevel = texture.m_MipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		cmdBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags{},
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		texture.m_CurrentLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		
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
