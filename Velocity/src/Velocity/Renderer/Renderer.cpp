#include "velpch.h"

#include "Renderer.hpp"
#include <Velocity/Core/Log.hpp>

#include <Velocity/Core/Window.hpp>
#include <Velocity/Core/Application.hpp>

#include <Velocity/Renderer/Swapchain.hpp>
#include <Velocity/Renderer/Shader.hpp>
#include <Velocity/Renderer/Pipeline.hpp>
#include <Velocity/Renderer/Vertex.hpp>
#include <Velocity/Renderer/BufferManager.hpp>

namespace Velocity
{
	std::shared_ptr<Renderer> Renderer::s_Renderer = nullptr;

	// Initalises Vulkan so it is ready to render
	Renderer::Renderer()
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapchain();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateBufferManager();
		CreateCommandBuffers();
		CreateSyncronizer();
	}

	// Submits a renderer command to be done
	void Renderer::Submit(BufferManager::Renderable object)
	{
		// TODO: Add logic
		m_SceneData.push_back(object);
	}
	
	// Takes all the information submitted this frame, records and submits the commands
	// Called by application in the run loop
	void Renderer::Render()
	{
		// Wait on fences
		// TODO: check result
		auto result = m_LogicalDevice->waitForFences(1, &m_Syncronizer.InFlightFences.at(m_CurrentFrame).get(), VK_TRUE, UINT64_MAX);
		
		// Acquire the next available image and signal the semaphore when one is
		vk::Result acquireResult{};
		m_CurrentImage = m_Swapchain->AcquireImage(UINT64_MAX, m_Syncronizer.ImageAvailable.at(m_CurrentFrame), &acquireResult);
		if (acquireResult == vk::Result::eSuboptimalKHR)
		{
			// This is recreating the swapchain
			OnWindowResize();
		}
	
		// Check if we need to wait on this image
		if (m_Syncronizer.ImagesInFlight.at(m_CurrentImage) != vk::Fence(nullptr))
		{
			// TODO: check result
			result = m_LogicalDevice->waitForFences(1, &m_Syncronizer.ImagesInFlight.at(m_CurrentImage), VK_TRUE, UINT64_MAX);
		}

		// Mark we are using
		m_Syncronizer.ImagesInFlight.at(m_CurrentImage) = m_Syncronizer.InFlightFences.at(m_CurrentFrame).get();
		
		// Submit command buffer

		// Records everything submitted
		RecordCommandBuffers();

		// Which semaphores to wait on before starting submission
		std::array<vk::Semaphore,1> waitSemaphores = { m_Syncronizer.ImageAvailable.at(m_CurrentFrame).get() };

		// Which semaphores to signal when submission is complete
		std::array<vk::Semaphore, 1> signalSemaphores = { m_Syncronizer.RenderFinished.at(m_CurrentFrame).get() };

		// Which stages of the pipeline wait to finish before we submit
		std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eTopOfPipe };
		
		// Combine all above data
		vk::SubmitInfo submitInfo = {
			static_cast<uint32_t>(waitSemaphores.size()),
			waitSemaphores.data(),
			waitStages.data(),
			1,
			&m_CommandBuffers.at(m_CurrentImage).get(),
			1,
			signalSemaphores.data()
		};
		// TODO: check result
		result = m_LogicalDevice->resetFences(1, &m_Syncronizer.InFlightFences.at(m_CurrentFrame).get());

		// Submit
		result = m_GraphicsQueue.submit(1, &submitInfo,m_Syncronizer.InFlightFences.at(m_CurrentFrame).get());
		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ASSERT(false, "Failed to submit command buffer!");
			VEL_CORE_ERROR("Failed to submit command buffer!");
		}

		// Time to present
		vk::PresentInfoKHR presentInfo = {
			1,
			signalSemaphores.data(),
			1,
			&m_Swapchain->GetSwapchainRaw(),
			&m_CurrentImage,
			nullptr
		};
		// TODO: check result
		result = m_PresentQueue.presentKHR(&presentInfo);

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	// Called by app when resize occurs
	void Renderer::OnWindowResize()
	{

		
		// Wait until device is done
		m_LogicalDevice->waitIdle();

		// Cleanup
		m_Swapchain.reset();

		// Free command buffers
		for (auto& buffer : m_CommandBuffers)
		{
			buffer.reset();
		}
		
		// Reset pipeline
		m_GraphicsPipeline.reset();

		// Now remake everything we need to
		CreateSwapchain();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandBuffers();
	}

	#pragma region INITALISATION FUNCTIONS
	// Creates the primary vulkan instance and prints out supported system extensions
	void Renderer::CreateInstance()
	{
		// First we need to check we can run if we are in debug and need validation layers
		// This will be stripped in release mode
		if (ENABLE_VALIDATION_LAYERS)
		{
			VEL_CORE_ASSERT(CheckValidationLayerSupport(), "Validation layers requested but are not available!");
		}

		// Create the application information
		vk::ApplicationInfo appInfo(
			"Velocity Renderer",
			VK_MAKE_VERSION(1, 0, 0),
			"No Engine",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_0
		);

		// Information on how to create the instance
		vk::InstanceCreateInfo createInfo{};
		createInfo.pApplicationInfo = &appInfo;

		// Get required extensions (depends on DEBUG/RELEASE)
		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// We only want validation if we are in debug mode
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		// Create and verify instance
		try
		{
			m_Instance = vk::createInstanceUnique(createInfo, nullptr);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, e.what());
			VEL_CORE_ERROR("An error occured in Renderer: {0}", e.what());
		}

		m_InstanceLoader = vk::DispatchLoaderDynamic(m_Instance.get(), vkGetInstanceProcAddr);

		// Check and display other exceptions
		uint32_t extensionCount = 0;
		if (vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) == vk::Result::eSuccess)
		{
			std::vector<vk::ExtensionProperties> extensions(extensionCount);
			if (vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) == vk::Result::eSuccess)
			{
				VEL_CORE_INFO("Available extensions:");
				for (const auto& extension : extensions)
				{
					VEL_CORE_TRACE("\t {0}", extension.extensionName);
				}
			}
		}
		VEL_CORE_INFO("Created Vulkan Instance!");

	}

	// Sets up the debug messenger callback
	void Renderer::SetupDebugMessenger()
	{
		// Nothing to do in release
		if (!ENABLE_VALIDATION_LAYERS)
		{
			return;
		}

		// Populate the info
		vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);

		// Create and verify the messenger
		try
		{
			m_DebugMessenger = m_Instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr, m_InstanceLoader);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create debug messenger! Error {0}", e.what());
			VEL_CORE_ERROR("An error occured in Renderer: {0}", e.what());
		}

		VEL_CORE_INFO("Created debug messenger!");
	}

	// Creates a window surface. Needs to be done to influence device picking
	void Renderer::CreateSurface()
	{
		VkSurfaceKHR tmpSurface = {};

		if (glfwCreateWindowSurface(m_Instance.get(), Application::GetWindow()->GetNative(), nullptr, &tmpSurface) != VK_SUCCESS)
		{
			VEL_CORE_ASSERT(false, "Failed to create swapchain window surface!");
			VEL_CORE_ERROR("Failed to create window surface!");
		}

		m_Surface = vk::UniqueSurfaceKHR(tmpSurface, m_Instance.get());

		VEL_CORE_INFO("Created window surface!");
	}

	// Selects a physical GPU from the PC
	void Renderer::PickPhysicalDevice()
	{
		// Prepare list of devices
		std::vector<vk::PhysicalDevice> devices;
		// Get list
		try
		{
			devices = m_Instance->enumeratePhysicalDevices();
		}
		catch (vk::SystemError& e)
		{
			// Assert in debug
			VEL_CORE_ASSERT(false, "Failed to find any GPUs with Vulkan support! Message:{0}", e.what());
			// Log in release
			VEL_CORE_ERROR("Failed to find any GPUs with Vulkan support! Message:{0}", e.what());
		}

		bool foundGPU = false;
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				foundGPU = true;
				break;
			}
		}

		if (!foundGPU)
		{
			// Assert in debug
			VEL_CORE_ASSERT(false, "Failed to find a suitable GPU with Vulkan support!");
			// Log in release
			VEL_CORE_ERROR("Failed to find a suitable GPU with Vulkan support!");
		}

		auto chosenProps = m_PhysicalDevice.getProperties();
		VEL_CORE_INFO("Selected GPU: {0}", chosenProps.deviceName);
	}

	// Creates a Vulkan logical device to interface with the physical device
	void Renderer::CreateLogicalDevice()
	{
		// Get the indices
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		// Prepare create info for queues
		// Currently graphics & present
		// They are likely the same but it is not guarenteed
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

		// This will knock them down to 1 value if they are the same
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.GraphicsFamily.value(),
			indices.PresentFamily.value()
		};

		float queuePriority = 1.0f;
		for (auto queueFamily : uniqueQueueFamilies)
		{
			vk::DeviceQueueCreateInfo createInfo{};
			createInfo.queueFamilyIndex = queueFamily;
			createInfo.queueCount = 1;
			createInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(createInfo);
		}

		// Prepare physical device features we need
		vk::PhysicalDeviceFeatures deviceFeatures{};

		// Prepare create info for the logical device
		vk::DeviceCreateInfo createInfo{};

		// Set queues
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		
		// Set required features
		createInfo.pEnabledFeatures = &deviceFeatures;

		// Set enabled extensions
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		
		// Set validation layers if we are using them
		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0u;
		}

		// Create device
		try
		{
			m_LogicalDevice = m_PhysicalDevice.createDeviceUnique(createInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create logical device! Error {0}", e.what());
			VEL_CORE_ERROR("An error occured in creating the logical device: {0}", e.what());
		}

		m_GraphicsQueue = m_LogicalDevice->getQueue(indices.GraphicsFamily.value(), 0);
		m_PresentQueue = m_LogicalDevice->getQueue(indices.PresentFamily.value(), 0);

		VEL_CORE_INFO("Created Logical device!");

	}

	// Creates the swapchain (creates chain, gets and makes images & views)
	void Renderer::CreateSwapchain()
	{
		auto support = QuerySwapchainSupport(m_PhysicalDevice);

		// Select optimal parameters
		vk::SurfaceFormatKHR surfaceFormat = Swapchain::ChooseFormat(support.Formats);
		vk::PresentModeKHR presentMode = Swapchain::ChoosePresentMode(support.PresentModes);
		vk::Extent2D extent = Swapchain::ChooseExtent(support.Capabilities);

		// Calculate other parameters
		uint32_t imageCount = support.Capabilities.minImageCount + 1;
		if (support.Capabilities.maxImageCount > 0 && imageCount > support.Capabilities.maxImageCount)
		{
			imageCount = support.Capabilities.maxImageCount;
		}

		// If our queue families are seperate we need to set the swapchain up differently
		// Assume default values and check for if we need to switch
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
		uint32_t indexCount = 0;
		uint32_t* indicesPtr = nullptr;

		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t qfIndices[] = { indices.GraphicsFamily.value(),indices.PresentFamily.value() };

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			sharingMode = vk::SharingMode::eConcurrent;
			indexCount = 2;
			indicesPtr = qfIndices;
		}

		// Set up the structure of doom
		vk::SwapchainCreateInfoKHR createInfo{};

		createInfo.surface = m_Surface.get();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		createInfo.imageSharingMode = sharingMode;
		createInfo.queueFamilyIndexCount = indexCount;
		createInfo.pQueueFamilyIndices = indicesPtr;
		createInfo.preTransform = support.Capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = nullptr;

		m_Swapchain = std::make_unique<Swapchain>(m_Surface, m_LogicalDevice, createInfo);

		VEL_CORE_INFO("Created swapchain with size:({0},{1})", extent.width, extent.height);
	}

	// Chonky function that creates a full pipeline
	void Renderer::CreateGraphicsPipeline()
	{
		// Load shaders as spv bytecode
		vk::ShaderModule vertShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/vert.spv");
		vk::ShaderModule fragShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/frag.spv");

		VEL_CORE_INFO("Loaded default shaders!");
		
		#pragma region CREATE SHADER MODULES
		
		// 1. Vertex
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

		#pragma region VERTEX INPUT

		// Sets up the pipeline to accept a Velocity::Vertex
		auto bindingDescription = Vertex::GetBindingDescription();
		auto attribDescription = Vertex::GetAttributeDescriptions();
		
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
				vk::PipelineVertexInputStateCreateFlags{},
				1,
				&bindingDescription,
				static_cast<uint32_t>(attribDescription.size()),
			attribDescription.data()
		};

		#pragma endregion

		#pragma region INPUT ASSEMBLY
		
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {
			vk::PipelineInputAssemblyStateCreateFlags{},
			vk::PrimitiveTopology::eTriangleList,
			VK_FALSE
		};

		#pragma endregion

		#pragma region VIEWPORT AND SCISSORS

		vk::Viewport viewport = {
			0.0f,
			0.0f,
			m_Swapchain->GetWidthF(),
			m_Swapchain->GetHeightF(),
			0.0f,
			1.0f
		};

		vk::Rect2D scissor = {
			{0,0},
			m_Swapchain->GetExtent()
		};

		vk::PipelineViewportStateCreateInfo viewportState = {
			vk::PipelineViewportStateCreateFlags{},
			1,
			&viewport,
			1,
			&scissor
		};
		
		#pragma endregion 

		#pragma region RASTERIZER

		vk::PipelineRasterizationStateCreateInfo rasterizer = {
			vk::PipelineRasterizationStateCreateFlags{},
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eClockwise,
			VK_FALSE,
			0.0f,
			0.0f,
			0.0f,
			1.0f
		};
		
		#pragma endregion

		#pragma region MULTISAMPLING

		vk::PipelineMultisampleStateCreateInfo multiSampling = {
			vk::PipelineMultisampleStateCreateFlags{},
			vk::SampleCountFlagBits::e1,
			VK_FALSE,
			1.0f,
			nullptr,
			VK_FALSE,
			VK_FALSE
		};
		
		#pragma endregion

		#pragma region DEPTH AND STENCIL
		#pragma endregion

		#pragma region COLOR BLENDING

		// Per framebuffer and we only have one atm
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
			VK_FALSE,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};

		// Global settings
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

		#pragma region DYNAMIC STATE

		std::array<vk::DynamicState, 2> dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eLineWidth
		};

		vk::PipelineDynamicStateCreateInfo dynamicState = {
			vk::PipelineDynamicStateCreateFlags{},
			dynamicStates
		};
		
		#pragma endregion 
		
		#pragma region PIPELINE LAYOUT

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
			vk::PipelineLayoutCreateFlags{},
			0,
			nullptr,
			0,
			nullptr
		};
		
		#pragma endregion

		#pragma region RENDER PASSES

		// Single colour buffer attachment (Relates to framebuffer)s
		vk::AttachmentDescription colorAttachment = {
			vk::AttachmentDescriptionFlags{},
			m_Swapchain->GetFormat(),
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		};

		// Single subpass for now
		// Subpasses can be combined into a single renderpass for better memory usage and optimisation

		// An attachment reference must reference an attachment described with an AttachmentDescription above
		// This is done with an index
		vk::AttachmentReference colorAttachmentRef = {
			0,
			vk::ImageLayout::eColorAttachmentOptimal
		};

		vk::SubpassDescription subpass = {
			vk::SubpassDescriptionFlags{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&colorAttachmentRef,
			nullptr,
			nullptr,
			0,
			nullptr
		};

		/* We make the render pass wait on the image being acquired as there is an implicit subpass at the start
		 this transisitons the image and will be wrong if we leave it unsorted */
		vk::SubpassDependency implicitDependency = {
			VK_SUBPASS_EXTERNAL,
			0,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlags{},
			vk::AccessFlagBits::eColorAttachmentWrite
		};
		

		vk::RenderPassCreateInfo renderPassInfo = {
			vk::RenderPassCreateFlags{},
			1,
			&colorAttachment,
			1,
			&subpass,
			1,
			&implicitDependency
		};
		
		#pragma endregion

		#pragma region PIPELINE CREATION

		vk::GraphicsPipelineCreateInfo pipelineInfo = {
			vk::PipelineCreateFlags{},
			static_cast<uint32_t>(shaderStages.size()),
			shaderStages.data(),
			&vertexInputInfo,
			&inputAssembly,
			nullptr,
			&viewportState,
			&rasterizer,
			&multiSampling,
			nullptr,
			&colorBlending,
			nullptr,				// No dynamic state yet
			nullptr,				// Pipeline Layout Supplied in pipeline constructor
			nullptr,				// Render pass supplied in pipeline constructor
			0,
			nullptr
		};

		m_GraphicsPipeline = std::make_unique<Pipeline>(m_LogicalDevice, pipelineInfo, pipelineLayoutInfo, renderPassInfo);

		VEL_CORE_INFO("Created graphics pipeline!");
		
		#pragma endregion
		
		// Modules hooked into the pipeline so we can delete here
		m_LogicalDevice->destroyShaderModule(vertShaderModule);
		m_LogicalDevice->destroyShaderModule(fragShaderModule);

	}

	// Creates framebuffers using the pipeline render pass details and the swapchain
	void Renderer::CreateFramebuffers()
	{
		// Each swapchain image needs a framebuffer
		m_Framebuffers.resize(m_Swapchain->GetImageViews().size());
		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); ++i)
		{
			vk::FramebufferCreateInfo framebufferInfo = {
				vk::FramebufferCreateFlags{},
				m_GraphicsPipeline->GetRenderPass().get(),
				1,
				&m_Swapchain->GetImageViews().at(i),
				m_Swapchain->GetWidth(),
				m_Swapchain->GetHeight(),
				1
			};
			try
			{
				m_Framebuffers.at(i) = m_LogicalDevice->createFramebufferUnique(framebufferInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ASSERT(false, "Failed to create framebuffer! Error {0}", e.what());
				VEL_CORE_ERROR("An error occurred in creating the framebuffers: {0}", e.what());
			}
			
		}

		VEL_CORE_INFO("Created framebuffers!");
		
	}

	// Creates the pool we will use to create all command buffers
	void Renderer::CreateCommandPool()
	{
		auto qfIndices = FindQueueFamilies(m_PhysicalDevice);

		vk::CommandPoolCreateInfo poolInfo = {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			qfIndices.GraphicsFamily.value()
		};

		try
		{
			m_CommandPool = m_LogicalDevice->createCommandPoolUnique(poolInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create command pool! Error {0}", e.what());
			VEL_CORE_ERROR("An error occurred in creating the command pool: {0}", e.what());
		}

		VEL_CORE_INFO("Created command pool!");
	}

	// Creates the vertex and index buffers we need
	void Renderer::CreateBufferManager()
	{
		m_BufferManager = std::make_unique<BufferManager>(m_PhysicalDevice, m_LogicalDevice, m_CommandPool.get(), m_GraphicsQueue);
	}

	// Allocates one command buffer per framebuffer
	void Renderer::CreateCommandBuffers()
	{
		vk::CommandBufferAllocateInfo allocInfo = {
			m_CommandPool.get(),
			vk::CommandBufferLevel::ePrimary,
			static_cast<uint32_t>(m_Framebuffers.size())
		};

		try
		{
			m_CommandBuffers = m_LogicalDevice->allocateCommandBuffersUnique(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create command buffer! Error {0}", e.what());
			VEL_CORE_ERROR("An error occurred in creating the command buffer: {0}", e.what());
		}

		VEL_CORE_INFO("Allocated command buffers!");
	}

	// Creates all required sync primitives
	void Renderer::CreateSyncronizer()
	{
		// Create semaphores
		// Used for image and rendering syncing
		vk::SemaphoreCreateInfo semaphoreInfo = {
			vk::SemaphoreCreateFlags{}
		};

		// Used for frame syncing
		vk::FenceCreateInfo fenceInfo = {
			vk::FenceCreateFlagBits::eSignaled
		};

		m_Syncronizer.ImagesInFlight.resize(m_Swapchain->GetImages().size(),vk::Fence(nullptr));
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			try
			{
				m_Syncronizer.ImageAvailable.at(i) = m_LogicalDevice->createSemaphoreUnique(semaphoreInfo);
				m_Syncronizer.RenderFinished.at(i) = m_LogicalDevice->createSemaphoreUnique(semaphoreInfo);
				m_Syncronizer.InFlightFences.at(i) = m_LogicalDevice->createFenceUnique(fenceInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ASSERT(false, "Failed to create sync primitives! Error {0}", e.what());
				VEL_CORE_ERROR("An error occurred in creating the sync primitives: {0}", e.what());
			}

		}
		VEL_CORE_INFO("Created syncronisation primitives!");
	}
	#pragma endregion 

	#pragma region RENDERING FUNCTIONS

	// Takes all commands sent through Renderer::Submit and records the buffers for them
	void Renderer::RecordCommandBuffers()
	{
		// TODO: Remove/change this static recording when you create the submit logic
		auto& cmdBuffer = m_CommandBuffers.at(m_CurrentImage);
		
		// Start a command buffer recording
		vk::CommandBufferBeginInfo beginInfo = {
			vk::CommandBufferUsageFlags{},
			nullptr
		};

		try
		{
			cmdBuffer->begin(beginInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to start record commandbuffers! Error {0}", e.what());
			VEL_CORE_ERROR("An error occurred in starting recording commandbuffers: {0}", e.what());
		}

		// Now we make the draw commands
		// TODO: This will be based on the contents submitted
		
		// Set magenta clear color
		std::array<float, 4> clearColor = {
			1.0f, 0.0f, 1.0f, 1.0f
		};
		vk::ClearValue clearColorValue = vk::ClearColorValue(clearColor);

		// Begin render pass
		vk::RenderPassBeginInfo renderPassInfo = {
			m_GraphicsPipeline->GetRenderPass().get(),
			m_Framebuffers.at(m_CurrentImage).get(),
			vk::Rect2D{{0,0},m_Swapchain->GetExtent()},
			1,
			&clearColorValue
		};
		cmdBuffer->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// 1. Bind pipeline and buffer
		m_GraphicsPipeline->Bind(cmdBuffer);
		m_BufferManager->Bind(cmdBuffer.get());

		// 2. Draw
		for (auto object : m_SceneData)
		{
			cmdBuffer->drawIndexed(object.IndexCount, 1, object.IndexStart, object.VertexOffset, 0);		
		}

		// 3. End
		cmdBuffer->endRenderPass();

		// 4. Check
		try
		{
			cmdBuffer->end();
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to record commandbuffers! Error {0}", e.what());
			VEL_CORE_ERROR("An error occurred in recording commandbuffers: {0}", e.what());
		}
	}

	#pragma endregion 
	
	#pragma region HELPER FUNCTIONS
	// Checks for required validation layers
	bool Renderer::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		if (vk::enumerateInstanceLayerProperties(&layerCount, nullptr) == vk::Result::eSuccess)
		{
			std::vector<vk::LayerProperties> availableLayers(layerCount);
			if (vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data()) == vk::Result::eSuccess)
			{
				for (auto layerName : m_ValidationLayers)
				{
					bool layerFound = false;

					for (const auto& properties : availableLayers)
					{
						if (strcmp(layerName, properties.layerName) == 0)
						{
							layerFound = true;
							break;
						}
					}

					if (!layerFound)
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	// Returns a list of required extensions dependant on if Validation Layers are needed
	std::vector<const char*> Renderer::GetRequiredExtensions()
	{
		// Setup required GLFW extensions
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (ENABLE_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
		
	}

	// This is fucking horrible. But its a way to hook into debug events and send them through Velocity
	VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		{
			return VK_FALSE;
		}

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		{
			VEL_CORE_TRACE("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		{
			VEL_CORE_INFO("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		{
			VEL_CORE_WARN("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		{
			VEL_CORE_ERROR("Validation Layer: {0}", pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
		}

		return VK_FALSE;
	}

	// Fills in the structure with the data for a debug messenger
	void Renderer::PopulateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		createInfo.pfnUserCallback = Renderer::DebugCallback;
		createInfo.pUserData = nullptr; // Optional
	}

	// Checks if a physical device is suitable
	bool Renderer::IsDeviceSuitable(vk::PhysicalDevice device)
	{
		auto hasIndices = FindQueueFamilies(device).IsComplete();

		auto hasExtensions = CheckDeviceExtensionsSupport(device);

		bool hasSwapchain = false;
		if (hasExtensions)
		{
			auto swapChainSupport = QuerySwapchainSupport(device);
			hasSwapchain = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		return hasIndices && hasExtensions && hasSwapchain;
	}

	// Checks if a device has required extensions
	bool Renderer::CheckDeviceExtensionsSupport(vk::PhysicalDevice device)
	{
		// Get all available extensions
		auto availableExtensions = device.enumerateDeviceExtensionProperties();

		// Get a set of our needed extensions
		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
		
		// Loop and erase all our of available extensions
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		// If empty they are all supported
		return requiredExtensions.empty();


	}

	// Checks if a device can support our swapchain requirements
	Renderer::SwapChainSupportDetails Renderer::QuerySwapchainSupport(vk::PhysicalDevice device)
	{
		SwapChainSupportDetails details;

		details.Capabilities = device.getSurfaceCapabilitiesKHR(m_Surface.get());
		details.Formats = device.getSurfaceFormatsKHR(m_Surface.get());
		details.PresentModes = device.getSurfacePresentModesKHR(m_Surface.get());

		return details;

	}

	// Finds the required queue families for the device
	Renderer::QueueFamilyIndices Renderer::FindQueueFamilies(vk::PhysicalDevice device)
	{
		// This is what we will return
		QueueFamilyIndices indices;

		// Get the queue familes
		auto queueFamilies = device.getQueueFamilyProperties();

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.GraphicsFamily = i;
			}

			if (device.getSurfaceSupportKHR(i, m_Surface.get()))
			{
				indices.PresentFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}

			++i;
		}

		return indices;


	}

	#pragma endregion

	// Destroys all vulkan data
	Renderer::~Renderer()
	{
		// Need to force this to happen before the other variables go out of scope
		m_Swapchain.reset();
	}
}
