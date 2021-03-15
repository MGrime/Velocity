#include "velpch.h"

#include "IBLMap.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "BaseBuffer.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "stb_image.h"
#include "Texture.hpp"
#include "Velocity/Core/Log.hpp"

namespace Velocity
{
	IBLMap::IBLMap(const std::string& filepath, vk::UniqueDevice& device, vk::PhysicalDevice& pDevice,
		vk::CommandPool& pool, uint32_t& graphicsQueueIndex, BufferManager& modelBuffer)
	{
		// Store references
		r_Device = &device;
		r_PhysicalDevice = pDevice;
		r_CommandPool = &pool;
		r_GraphicsQueueIndex = graphicsQueueIndex;
		
		#pragma region LOAD HDRI FILE

		int width, height, nrComponents;
		// Force an alpha channel
		float* imgData = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, STBI_rgb_alpha);

		if (!imgData)
		{
			VEL_CORE_ERROR("Failed to open HDRI file {0}", filepath);
			VEL_CORE_ASSERT(false, "Failed to load HDRI file");
			return;
		}

		// Calculate size
		const VkDeviceSize imageSize = width * height * (sizeof(float) * 4);

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
			vk::Format::eR32G32B32A32Sfloat,
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
				vk::Format::eR32G32B32A32Sfloat,
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
				vk::Format::eR32G32B32A32Sfloat,
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
			vk::Format::eR32G32B32A32Sfloat,
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
		EquirectangularToCubemap(equirectangularImageView, modelBuffer);

		// Save data for renderer
		m_EnviromentMapImageInfo = {
			m_Sampler.get(),
			m_EnviromentMapImageView,
			vk::ImageLayout::eShaderReadOnlyOptimal
		};

		m_EnviromentMapWriteSet = {
			nullptr,
			1,
			0,
			1,
			vk::DescriptorType::eCombinedImageSampler,
			&m_EnviromentMapImageInfo,
			nullptr,
			nullptr
		};

		Renderer::GetRenderer()->LoadMesh("../Velocity/assets/models/sphere.obj", "VEL_INTERNAL_Skybox");

		m_SphereMesh = MeshComponent{ "VEL_INTERNAL_Skybox" };

		VEL_CORE_INFO("Created HDR skybox from file {0}", filepath);
		
	}

	IBLMap::~IBLMap()
	{
		r_Device->get().destroyImage(m_EnviromentMapImage);
		r_Device->get().destroyImageView(m_EnviromentMapImageView);
		r_Device->get().freeMemory(m_EnviromentMapMemory);
	}

	void IBLMap::EquirectangularToCubemap(vk::UniqueImageView& equirectangularIV, BufferManager& modelBuffer)
	{
		// This function is MASSIVE. Creates a pipeline to be used one time to perform this operation
		// Firstly we create the image that will be the end result leaving it in transfer dst mod
		// Then we create the pipeline with the flattocubemap shaders, rendering 6 passes to 6 framebuffers
		// Then each framebuffer is copied to a layer of the end result

		// Scale each side to 512. 
		const uint32_t mapFaceSize = 512u;

		#pragma region CREATE VULKAN IMAGE
		// Stored as a cube map (image with 6 array layers) with 16bit floating point values
		// Down sample to 2k
		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlagBits::eCubeCompatible,
			vk::ImageType::e2D,
			vk::Format::eR16G16B16A16Sfloat,
			vk::Extent3D{mapFaceSize,mapFaceSize,1},
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
		vk::MemoryRequirements memRequirements = r_Device->get().getImageMemoryRequirements(m_EnviromentMapImage);

		vk::MemoryAllocateInfo allocInfo = {
			memRequirements.size,
			BaseBuffer::FindMemoryType(r_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		try
		{
			m_EnviromentMapMemory = r_Device->get().allocateMemory(allocInfo);

		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load hdri: Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to load hdri: Error: {0}", e.what());
			return;
		}
		
		#pragma endregion

		// Bind
		r_Device->get().bindImageMemory(m_EnviromentMapImage, m_EnviromentMapMemory, 0);
		
		#pragma region PROCESS RAW TO VULKAN TRANSFER DST
		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer processBufferWrapper = TemporaryCommandBuffer(*r_Device, *r_CommandPool, queue);
			auto& processBuffer = processBufferWrapper.GetBuffer();

			// Set this image to destination optimal
			Texture::TransitionImageLayout(processBuffer, m_EnviromentMapImage, vk::Format::eR16G16B16A16Sfloat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 6);
		}
		#pragma endregion

		#pragma region CREATE PIPELINE (and associated)

		#pragma region LOAD SHADERS
		vk::ShaderModule vertShaderModule = Shader::CreateShaderModule(*r_Device, "../Velocity/assets/shaders/ibl/flattocubemapvert.spv");
		vk::ShaderModule fragShaderModule = Shader::CreateShaderModule(*r_Device, "../Velocity/assets/shaders/ibl/flattocubemapfrag.spv");

		vk::PipelineShaderStageCreateInfo vertexShaderStageInfo = {
			vk::PipelineShaderStageCreateFlags{0u},
			vk::ShaderStageFlagBits::eVertex,
			vertShaderModule,
			"main"
		};
		// 2. Fragment
		vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo = {
			vk::PipelineShaderStageCreateFlags{0u},
			vk::ShaderStageFlagBits::eFragment,
			fragShaderModule,
			"main"
		};
		// Combine into a contiguous structure
		std::array<vk::PipelineShaderStageCreateInfo, 2u> shaderStages = { vertexShaderStageInfo, fragmentShaderStageInfo };
		
		#pragma endregion

		#pragma region CREATE RENDER PASS
		// Framebuffer will be attached and left read to transfer to image above
		vk::AttachmentDescription attDesc = {
			vk::AttachmentDescriptionFlags{},
			vk::Format::eR16G16B16A16Sfloat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferSrcOptimal
		};
		vk::AttachmentReference colorRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		// Define subpass
		vk::SubpassDescription subpassDesc = {
			vk::SubpassDescriptionFlags{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&colorRef,
			nullptr,
			nullptr,
			0,
			nullptr
		};

		// Use subpass dependencies for layout transitions
		std::array<vk::SubpassDependency, 2> dependencies;
		// Transition from undefined to color attachment
		dependencies.at(0) = {
			VK_SUBPASS_EXTERNAL,
			0,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eMemoryRead,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		};
		// Transition from color attachment to transfer src
		dependencies.at(1) = {
			0,
			VK_SUBPASS_EXTERNAL,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eTransfer,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlags{},
			vk::DependencyFlagBits::eByRegion
		};

		// Create render pass
		vk::RenderPassCreateInfo renderPassInfo = {
			vk::RenderPassCreateFlags{},
			1,
			&attDesc,
			1,
			&subpassDesc,
			2,
			dependencies.data()
		};

		vk::RenderPass renderpass;
		try
		{
			renderpass = r_Device->get().createRenderPass(renderPassInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load hdri: Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to load hdri: Error: {0}", e.what());
			return;
		}

		#pragma endregion

		#pragma region CREATE FRAMEBUFFER RESOURCES

		std::array<vk::Image, 6>		framebufferImages;
		std::array<vk::DeviceMemory, 6> framebufferMemories;
		std::array<vk::ImageView, 6>	framebufferImageViews;
		for (size_t i = 0; i < 6; ++i)
		{
			// Image
			vk::ImageCreateInfo fbInfo = {
				vk::ImageCreateFlags{},
				vk::ImageType::e2D,
				vk::Format::eR16G16B16A16Sfloat,
				vk::Extent3D {
					mapFaceSize,
					mapFaceSize,
					1
				},
				1,
				1,
				vk::SampleCountFlagBits::e1,
				vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
				vk::SharingMode::eExclusive,
				1,
				&r_GraphicsQueueIndex,
				vk::ImageLayout::eUndefined
			};
			try
			{
				framebufferImages.at(i) = r_Device->get().createImage(fbInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the HDRI framebuffer images: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create HDRI framebuffer images! Error {0}", e.what());
				return;
			}
			
			// Memory
			memRequirements = r_Device->get().getImageMemoryRequirements(framebufferImages.at(i));

			allocInfo = {
				memRequirements.size,
				BaseBuffer::FindMemoryType(r_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
			};

			try
			{
				framebufferMemories.at(i) = r_Device->get().allocateMemory(allocInfo);

			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the hdri framebuffer images memory: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create hdri framebuffer images memory! Error {0}", e.what());
				return;
			}

			r_Device->get().bindImageMemory(framebufferImages.at(i), framebufferMemories.at(i), 0);

			// View
			vk::ImageViewCreateInfo fbView = {
				vk::ImageViewCreateFlags{},
				framebufferImages.at(i),
				vk::ImageViewType::e2D,
				vk::Format::eR16G16B16A16Sfloat,
				{},
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
				framebufferImageViews.at(i) = r_Device->get().createImageView(fbView);

			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the hdri framebuffer image views: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create framebuffer hdri image views! Error {0}", e.what());
				return;
			}
			
		}
		
		#pragma endregion

		#pragma region CREATE FRAMEBUFFERS

		std::array<vk::Framebuffer, 6> framebuffers;
		for (size_t i = 0; i < 6; ++i)
		{
			vk::FramebufferCreateInfo framebufferInfo = {
				vk::FramebufferCreateFlags{},
				renderpass,
				1,
				&framebufferImageViews.at(i),
				mapFaceSize,
				mapFaceSize,
				1
			};

			try
			{
				framebuffers.at(i) = r_Device->get().createFramebuffer(framebufferInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("Failed to create HDRI framebuffers! Error: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create HDRI framebuffers: {0}", e.what());
				return;
			}
		}
		
		
		#pragma endregion

		#pragma region CREATE DESCRIPTORS
		// Set layout
		vk::DescriptorSetLayout descriptorSetLayout;
		vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding = {
			0,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr
		};
		vk::DescriptorSetLayoutCreateInfo descriptorSetCreateInfo = {
			vk::DescriptorSetLayoutCreateFlags{},
			1,
			&descriptorSetLayoutBinding
		};

		try
		{
			descriptorSetLayout = r_Device->get().createDescriptorSetLayout(descriptorSetCreateInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to create HDRI descriptor set layout. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create HDRI descriptor set layout", e.what());
		}

		// Pool
		vk::DescriptorPoolSize poolSize = { vk::DescriptorType::eCombinedImageSampler,1 };
		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo = {
			vk::DescriptorPoolCreateFlags{},
			6,
			1,
			&poolSize
		};
		vk::DescriptorPool descriptorPool;
		try
		{
			descriptorPool =
				r_Device->get().createDescriptorPool(descriptorPoolCreateInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to create HDRI descriptor pool. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create HDRI descriptor pool", e.what());
		}

		// Sets
		vk::DescriptorSet descriptorSet;
		vk::DescriptorSetAllocateInfo descriptorSetAllocInfo = {
			descriptorPool,
			1,
			&descriptorSetLayout
		};

		auto result = r_Device->get().allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorSet);

		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ERROR("Failed to allocate HDRI descriptor set");
			VEL_CORE_ASSERT(false, "Failed to allocate HDRI descriptor set");
		}

		// Write set
		vk::DescriptorImageInfo equirectangularInfo = {
			m_Sampler.get(),
			equirectangularIV.get(),
			vk::ImageLayout::eShaderReadOnlyOptimal
		};
		
		vk::WriteDescriptorSet writeSet = {
			descriptorSet,0,0,
			1,vk::DescriptorType::eCombinedImageSampler,
			&equirectangularInfo,nullptr,nullptr
		};

		r_Device->get().updateDescriptorSets(1, &writeSet, 0, nullptr);
		
		#pragma endregion

		#pragma region PUSH CONSTANT RANGE

		vk::PushConstantRange vertPushConstantRange = {
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4) * 2
		};
		
		#pragma endregion

		#pragma region CREATE PIPELINE LAYOUT
		vk::PipelineLayout pipelineLayout;
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			vk::PipelineLayoutCreateFlags{},
			1,
			&descriptorSetLayout,
			1,
			&vertPushConstantRange
		};

		try
		{
			pipelineLayout = r_Device->get().createPipelineLayout(pipelineLayoutCreateInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to create HDRI pipeline layout! Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create HDRI pipeline layout: {0}", e.what());
			return;
		}
		
		#pragma endregion

		#pragma region FIXED FUNCTION
		// Vert input
		auto bindingDescription = Vertex::GetBindingDescription();
		auto attribDescription = Vertex::GetAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
			vk::PipelineVertexInputStateCreateFlags{},
			1,
			&bindingDescription,
			static_cast<uint32_t>(attribDescription.size()),
			attribDescription.data()
		};
		// Input assembly
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {
			vk::PipelineInputAssemblyStateCreateFlags{},
			vk::PrimitiveTopology::eTriangleList,
			VK_FALSE
		};
		// Viewport and scissor
		vk::Viewport viewport = {
			0.0f,0.0f,
			mapFaceSize,mapFaceSize,
			0.0f,1.0f
		};
		vk::Rect2D scissor = {
			vk::Offset2D{0,0},
			vk::Extent2D{mapFaceSize,mapFaceSize}
		};
		vk::PipelineViewportStateCreateInfo viewportState = {
			vk::PipelineViewportStateCreateFlags{},
			1,
			&viewport,
			1,
			&scissor
		};
		// Rasterizer TODO: Check the culling and orientation
		vk::PipelineRasterizationStateCreateInfo rasterizer = {
			vk::PipelineRasterizationStateCreateFlags{},
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			VK_FALSE,
			0.0f,0.0f,0.0f,1.0f
		};
		// Multisampling
		vk::PipelineMultisampleStateCreateInfo multisampling = {
			vk::PipelineMultisampleStateCreateFlags{},
			vk::SampleCountFlagBits::e1,
			VK_FALSE,
			1.0f,
			nullptr,
			VK_FALSE,
			VK_FALSE
		};
		// NO DEPTH
		// Color blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
			VK_FALSE,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd
		};
		std::array<float, 4> blendConstants = { 0.0f,0.0f,0.0f,0.0f };
		vk::PipelineColorBlendStateCreateInfo colorBlending = {
			vk::PipelineColorBlendStateCreateFlags{},
			VK_FALSE,
			vk::LogicOp::eCopy,
			1,
			&colorBlendAttachment,
			blendConstants
		};

		
		
		#pragma endregion

		#pragma region CREATE PIPELINE

		vk::GraphicsPipelineCreateInfo pipelineCI = {
			vk::PipelineCreateFlags{},
			2u,
			shaderStages.data(),
			&vertexInputInfo,
			&inputAssembly,
			nullptr,
			&viewportState,
			&rasterizer,
			&multisampling,
			nullptr,
			&colorBlending,
			nullptr,
			pipelineLayout,
			renderpass,
			0,
			nullptr
		};
		vk::Pipeline pipeline;

		result = r_Device->get().createGraphicsPipelines(nullptr, 1, &pipelineCI, nullptr, &pipeline);

		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ERROR("Failed to create HDRI pipeline (Stage 1)");
			VEL_CORE_ASSERT(false,"Failed to create HDRI pipeline (Stage 1)");
			return;
		}
		
		
		#pragma endregion
		
		#pragma endregion

		#pragma region FLAT TO CUBE
		// Now we can actually do the rendering process
		std::array<float, 4> clearColor = {
			1.0f, 0.0f, 1.0f, 1.0f
		};
		vk::ClearValue clearColorVal = vk::ClearValue(clearColor);
		// Create array of capture matricies
		// Each pass uses the projection and one of the views
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 512.0f);
		captureProjection[1][1] *= -1.0f;
		std::vector<glm::mat4> captureViews = {
			// POSITIVE_Y
			rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			// POSITIVE_X
			rotate(rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_X
			rotate(rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f))
		};

		for (auto& v : captureViews)
		{
			v[0][2] *= -1.0f;
			v[1][2] *= -1.0f;
			v[2][2] *= -1.0f;
			v[3][2] *= -1.0f;
		}
		
		// Scope the commandbuffer RAII object
		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer renderBufferWrapper = TemporaryCommandBuffer(*r_Device, *r_CommandPool, queue);
			auto& renderBuffer = renderBufferWrapper.GetBuffer();

			// Bind vertices
			modelBuffer.Bind(renderBuffer);

			Renderer::GetRenderer()->LoadMesh("../Velocity/assets/models/cube.obj", "VEL_INTERNAL_Cube");

			auto mesh = Renderer::GetRenderer()->GetMeshList().find("VEL_INTERNAL_Cube")->second;

			// For each side
			for (size_t i = 0; i < 6; ++i)
			{
				// Begin render pass
				vk::RenderPassBeginInfo renderPassInfo = {
					renderpass,
					framebuffers.at(i),
					vk::Rect2D{{0,0},vk::Extent2D{mapFaceSize,mapFaceSize}},
					1,
					&clearColorVal
				};
				renderBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

				// Bind pipeline
				renderBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

				// Bind descriptor set for equirectangular map
				renderBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// push matrices
				renderBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), value_ptr(captureProjection));
				renderBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(glm::mat4), sizeof(glm::mat4), value_ptr(captureViews[i]));

				// Draw cube
				renderBuffer.drawIndexed(mesh.IndexCount, 1, mesh.IndexStart, mesh.VertexOffset,0);

				// End render pass
				renderBuffer.endRenderPass();
				
			}
		}		
		#pragma endregion

		r_Device->get().waitIdle();

		// At this point the 6 frame buffer images should each contain 1 face of the flattened map
		// So we can copy it over
		// Scope the buffer
		{
			vk::Queue queue = r_Device->get().getQueue(r_GraphicsQueueIndex, 0);
			TemporaryCommandBuffer renderBufferWrapper = TemporaryCommandBuffer(*r_Device, *r_CommandPool, queue);
			auto& copyBuffer = renderBufferWrapper.GetBuffer();
			
			for (size_t i = 0; i < 6; ++i)
			{
				// Copy framebuffer to array layer
				vk::ImageCopy faceCopy = {
					vk::ImageSubresourceLayers{
						vk::ImageAspectFlagBits::eColor,
						0,
						0,
						1
					},
					vk::Offset3D{0,0,0},
					vk::ImageSubresourceLayers{
						vk::ImageAspectFlagBits::eColor,
						0,
						static_cast<uint32_t>(i),
						1
					},
					vk::Offset3D{0,0,0},
					vk::Extent3D{
						mapFaceSize,
						mapFaceSize,
						1
					}
				};

				copyBuffer.copyImage(
					framebufferImages.at(i),
					vk::ImageLayout::eTransferSrcOptimal,
					m_EnviromentMapImage,
					vk::ImageLayout::eTransferDstOptimal,
					1,
					&faceCopy
				);
			}

			// Now the image has the full cubemap but needs to transitioned to shader read
			Texture::TransitionImageLayout(
				copyBuffer,
				m_EnviromentMapImage,
				vk::Format::eR16G16B16A16Sfloat,
				vk::ImageLayout::eTransferDstOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				1,
				6
			);
		}

		// Delete all used resources
		r_Device->get().destroyShaderModule(vertShaderModule);
		r_Device->get().destroyShaderModule(fragShaderModule);
		
		r_Device->get().destroyPipeline(pipeline);
		r_Device->get().destroyPipelineLayout(pipelineLayout);
		r_Device->get().destroyRenderPass(renderpass);

		for (int i = 0; i < 6; ++i)
		{
			r_Device->get().destroyImage(framebufferImages.at(i));
			r_Device->get().destroyImageView(framebufferImageViews.at(i));
			r_Device->get().freeMemory(framebufferMemories.at(i));

			r_Device->get().destroyFramebuffer(framebuffers.at(i));
		}

		r_Device->get().destroyDescriptorSetLayout(descriptorSetLayout);
		r_Device->get().destroyDescriptorPool(descriptorPool);

		// Create image view for the cube map
		vk::ImageViewCreateInfo finalViewInfo = {
			vk::ImageViewCreateFlags{},
			m_EnviromentMapImage,
			vk::ImageViewType::eCube,
			vk::Format::eR16G16B16A16Sfloat,
			{},
			{
				vk::ImageAspectFlagBits::eColor,
				0,1,0,6
			}
		};
		try
		{
			m_EnviromentMapImageView = r_Device->get().createImageView(finalViewInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to load hdr skybox: {0} (Failed to create image view)");
			VEL_CORE_ASSERT(false, "Failed to load hdr skybox: {0} (Failed to create image view)");
		}
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
