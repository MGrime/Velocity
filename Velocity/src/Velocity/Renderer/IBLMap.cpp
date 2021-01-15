#include "velpch.h"

#include "IBLMap.hpp"


#include "BaseBuffer.hpp"
#include "stb_image.h"
#include "Texture.hpp"
#include "Velocity/Core/Log.hpp"

namespace Velocity
{
	IBLMap::IBLMap(const std::string& filepath, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice,
		vk::CommandPool& pool, uint32_t& graphicsQueueIndex)
	{
		// Store references
		r_Device = &device;
		r_PhysicalDevice = pDevice;
		r_CommandPool = &pool;
		r_GraphicsQueueIndex = graphicsQueueIndex;
		
		#pragma region LOAD HDRI FILE

		int width, height, nrComponents;
		float* imgData = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);

		if (!imgData)
		{
			VEL_CORE_ERROR("Failed to open HDRI file {0}", filepath);
			VEL_CORE_ASSERT(false, "Failed to load HDRI file");
			return;
		}

		// Calculate size
		const VkDeviceSize imageSize = width * height * (sizeof(float) * nrComponents);

		// Create staging buffer
		std::unique_ptr<BaseBuffer> stagingBuffer = std::make_unique<BaseBuffer>(
			r_PhysicalDevice,
			*r_Device,
			imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		// Map data
		void* data;
		auto result = r_Device->get().mapMemory(stagingBuffer->Memory.get(), 0, imageSize, vk::MemoryMapFlags{}, &data);
		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ERROR("Failed to load hdri: {0} (Failed to map memory)", filepath);
			VEL_CORE_ASSERT(false, "Failed to load hdri: {0} (Failed to map memory)", filepath);
			return;
		}
		// Copy over image data
		memcpy(data, imgData, imageSize);

		// Unmap
		r_Device->get().unmapMemory(stagingBuffer->Memory.get());

		// free stbi
		stbi_image_free(static_cast<void*>(imgData));
		
		#pragma endregion

		#pragma region CREATE VULKAN IMAGE

		vk::ImageCreateInfo equirectangularImageInfo = {
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			vk::Format::eR32G32B32Sfloat,
			vk::Extent3D{static_cast<uint32_t>(width),static_cast<uint32_t>(height),1},
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::SharingMode::eExclusive,
			{},
			{},
			vk::ImageLayout::eUndefined
		};

		// Local variable as we dont need this when the cubemaps are generated
		vk::UniqueImage equirectangularImage;

		try
		{
			equirectangularImage = r_Device->get().createImageUnique(equirectangularImageInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to create HDR: {0} (Failed to create image) Error: {1}", filepath, e.what());
			VEL_CORE_ASSERT(false,"Failed to create HDR: {0} (Failed to create image) Error: {1}", filepath, e.what());
			return;
		}
		
		#pragma endregion

		#pragma region ALLOCATE MEMORY

		vk::MemoryRequirements memRequirements = r_Device->get().getImageMemoryRequirements(equirectangularImage.get());

		vk::MemoryAllocateInfo allocInfo = {
			memRequirements.size,
			BaseBuffer::FindMemoryType(r_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		vk::UniqueDeviceMemory imageMemory;
		
		try
		{
			imageMemory = r_Device->get().allocateMemoryUnique(allocInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load HDRI: {0} (Failed to create memory) Error: {1}", filepath, e.what());
			VEL_CORE_ASSERT(false,"Failed to load HDRI: {0} (Failed to create memory) Error: {1}", filepath, e.what());
			return;
		}
		
		#pragma endregion

		// Bind image and memory
		r_Device->get().bindImageMemory(equirectangularImage.get(), imageMemory.get(), 0);

		#pragma region PROCESS RAW TO VULKAN
		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer processBufferWrapper = TemporaryCommandBuffer(*r_Device, pool, queue);
			auto& processBuffer = processBufferWrapper.GetBuffer();

			// Set to dest optimal
			Texture::TransitionImageLayout(
				processBuffer, equirectangularImage.get(),
				vk::Format::eR32G32B32Sfloat,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal,
				1,
				1
			);

			// Copy
			Texture::CopyBufferToImage(
				processBuffer,equirectangularImage.get(),
				stagingBuffer->Buffer.get(),
				width,height,
				1
			);

			// Set to shader sample
			Texture::TransitionImageLayout(
				processBuffer, equirectangularImage.get(),
				vk::Format::eR32G32B32Sfloat,
				vk::ImageLayout::eTransferDstOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				1,
				1
			);
		}
		#pragma endregion

		#pragma region CREATE IMAGE VIEW

		vk::ImageViewCreateInfo viewInfo = {
			vk::ImageViewCreateFlags{},
			equirectangularImage.get(),
			vk::ImageViewType::e2D,
			vk::Format::eR32G32B32Sfloat,
			{},
			{
				vk::ImageAspectFlagBits::eColor,
				0,1,0,1
			}
		};

		// Same as with image
		vk::UniqueImageView equirectangularImageView;

		try
		{
			equirectangularImageView = r_Device->get().createImageViewUnique(viewInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to create HDR: {0} (Failed to create image view) Error: {1}", filepath, e.what());
			VEL_CORE_ASSERT(false, "Failed to create HDR: {0} (Failed to create image view) Error: {1}", filepath, e.what());
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
			m_Sampler = r_Device->get().createSamplerUnique(sampler);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load HDRI: {0} (Failed to create sampler) Error: {1}", filepath, e.what());
			VEL_CORE_ASSERT(false, "Failed to load HDRI: {0} (Failed to create sampler) Error: {1}", filepath, e.what());
			return;
		}
		
		#pragma endregion

		// Convert the flat map into a cube map
		EquirectangularToCubemap(equirectangularImageView,width,height);
	}

	IBLMap::~IBLMap()
	{
		r_Device->get().destroyImage(m_EnviromentMapImage);
	}

	void IBLMap::EquirectangularToCubemap(vk::UniqueImageView& equirectangularIV, int width, int height)
	{
		// This function is MASSIVE. Creates a pipeline to be used one time to perform this operation
		// Firstly we create the image that will be the end result leaving it in transfer dst mod
		// Then we create the pipeline with the flattocubemap shaders, rendering 6 passes to 6 framebuffers
		// Then each framebuffer is copied to a layer of the end result

		#pragma region CREATE VULKAN IMAGE
		// Stored as a cube map (image with 6 array layers) with 16bit floating point values
		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlagBits::eCubeCompatible,
			vk::ImageType::e2D,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Extent3D{static_cast<uint32_t>(width),static_cast<uint32_t>(height),1},
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
			m_EnviromentMapImage = r_Device->get().createImage(imageInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to convert HDRI to cubemap Error: {0}", e.what());
			VEL_CORE_ASSERT(false,"Failed to convert HDRI to cubemap Error: {0}", e.what());
			return;
		}
		
		#pragma endregion
		#pragma region ALLOCATE MEMORY
		#pragma endregion
		#pragma region PROCESS RAW TO VULKAN TRANSFER DST
		#pragma endregion
		
	}

	void IBLMap::CreateIrradianceMap()
	{
	}

	void IBLMap::PrefilterCubemap()
	{
	}

	void IBLMap::ComputeBRDF()
	{
	}
}
