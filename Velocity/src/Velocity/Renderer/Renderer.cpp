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
#include <Velocity/Renderer/Texture.hpp>
#include <Velocity/Renderer/Skybox.hpp>

#include <Velocity/Utility/Camera.hpp>

#include "Velocity/ECS/Scene.hpp"
#include "Velocity/ECS/Entity.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "imgui.h"
#include <Velocity/ImGui/fonts/roboto.cpp>	// I know this is really odd but its how its done

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include "Velocity/Utility/Input.hpp"

#include "IBLMap.hpp"

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
		CreateGraphicsPipelines();
		CreateCommandPool();
		CreateColorResources();
		CreateDepthResources();
		CreateTextureSamplers();
		CreateFramebufferResources();
		CreateFramebuffers();
		CreateBufferManager();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncronizer();
		InitaliseImgui();

		// Make sure gizmo is set null
		m_GizmoEntity = nullptr;
	}

	#pragma region USER API
	
	// This is called when you want to start the rendering of a scene!
	void Renderer::SetScene(Scene* scene)
	{
		m_LogicalDevice->waitIdle();
		m_ActiveScene = scene;
	}

	// Submits a renderer command to be done
	// Returns a new texture.
	uint32_t Renderer::CreateTexture(const std::string& filepath, const std::string& referenceName)
	{
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		m_Textures.push_back({ referenceName,new Texture(filepath, m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value()) });
		auto newIndex = static_cast<uint32_t>(m_Textures.size()) - 1u;
		// Update the texture info
		m_TextureInfos.at(newIndex).imageView = m_Textures.back().second->m_ImageView.get();

		m_LogicalDevice->waitIdle();

		for (auto writes : m_DescriptorWrites)
		{
			m_LogicalDevice->updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

		}
		
		m_TextureGUIIDs.push_back((ImTextureID)ImGui_ImplVulkan_AddTexture(m_TextureSampler.get(), m_Textures.back().second->m_ImageView.get(), (VkImageLayout)m_Textures.back().second->m_CurrentLayout));
		
		return newIndex;

	}
	// Returns a material component
	PBRComponent Renderer::CreatePBRMaterial(const std::string& basefilepath, const std::string& extension, const std::string& referenceName, bool heightMapped)
	{
		PBRComponent newComponent;
		// Mark them as internal in the texture array so they arent displayed for non PBR texture adding
		newComponent.AlbedoID() = static_cast<int32_t>(CreateTexture(basefilepath + "_albedo" + extension, "VEL_INTERNAL_" + referenceName + "_albedo"));
		newComponent.NormalID() = static_cast<int32_t>(CreateTexture(basefilepath + "_normal" + extension, "VEL_INTERNAL_" + referenceName + "_normal"));
		if (heightMapped)
		{
			newComponent.HeightID() = static_cast<int32_t>(CreateTexture(basefilepath + "_height" + extension, "VEL_INTERNAL_" + referenceName + "_height"));
		}
		else
		{
			newComponent.HeightID() = -1;
		}
		newComponent.MetallicID() = static_cast<int32_t>(CreateTexture(basefilepath + "_metallic" + extension, "VEL_INTERNAL_" + referenceName + "_metallic"));
		newComponent.RoughnessID() = static_cast<int32_t>(CreateTexture(basefilepath + "_roughness" + extension, "VEL_INTERNAL_" + referenceName + "_roughness"));

		newComponent.MaterialName = referenceName;
		
		// Store component for user to access later
		m_PBRMaterials[referenceName] = newComponent;
		

		return newComponent;
		
	}

	// Returns a skybox
	Skybox* Renderer::CreateSkybox(const std::string& baseFilepath, const std::string& extension)
	{
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		return new Skybox(baseFilepath, extension, m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value());
	}

	// Returns a HDR Skybox
	IBLMap* Renderer::CreateHDRSkybox(const std::string& filepath)
	{
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		return new IBLMap(filepath, m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value(),*m_BufferManager.get());
	}

	#pragma endregion 
	
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
		// If enablegui = false it will also copy to the swapchain image
		RecordCommandBuffers();

		if (m_EnableGUI)
		{
			// Records submitted gui
			RecordImGuiCommandBuffers();
		}


		// Which semaphores to wait on before starting submission
		std::array<vk::Semaphore,1> waitSemaphores = { m_Syncronizer.ImageAvailable.at(m_CurrentFrame).get() };

		// Which semaphores to signal when submission is complete
		std::array<vk::Semaphore, 1> signalSemaphores = { m_Syncronizer.RenderFinished.at(m_CurrentFrame).get() };

		// Which stages of the pipeline wait to finish before we submit
		std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eTopOfPipe };

		// Update buffers
		UpdateUniformBuffers();
		
		// Combine all above data
		std::vector<vk::CommandBuffer> submitBuffer;
		submitBuffer.reserve(2);

		submitBuffer.push_back(m_CommandBuffers.at(m_CurrentImage).get());
		
		if (m_EnableGUI)
		{
			submitBuffer.push_back(m_ImGuiCommandBuffers.at(m_CurrentImage));
		}
		
		vk::SubmitInfo submitInfo = {
			static_cast<uint32_t>(waitSemaphores.size()),
			waitSemaphores.data(),
			waitStages.data(),
			static_cast<uint32_t>(submitBuffer.size()),
			submitBuffer.data(),
			1,
			signalSemaphores.data()
		};
		// TODO: check result
		result = m_LogicalDevice->resetFences(1, &m_Syncronizer.InFlightFences.at(m_CurrentFrame).get());

		// Submit
		result = m_GraphicsQueue.submit(1, &submitInfo,m_Syncronizer.InFlightFences.at(m_CurrentFrame).get());
		if (result != vk::Result::eSuccess)
		{
			VEL_CORE_ERROR("Failed to submit command buffer!");
			VEL_CORE_ASSERT(false, "Failed to submit command buffer!");
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

		// Flush key events
		Input::OnFrameFinished();
	}

	// Called by app when resize occurs
	void Renderer::OnWindowResize()
	{
		// Wait until device is done
		m_LogicalDevice->waitIdle();
		auto result = m_LogicalDevice->waitForFences(1, &m_Syncronizer.ImagesInFlight.at(m_CurrentFrame), VK_TRUE, UINT64_MAX);

		m_ColorImageView.reset();
		m_ColorImage.reset();
		m_ColorMemory.reset();
		
		m_DepthImageView.reset();
		m_DepthImage.reset();
		m_DepthMemory.reset();

		for (auto& view : m_FramebufferImageViews)
		{
			view.reset();
		}
		
		for (auto& image : m_FramebufferImages)
		{
			image.reset();
		}

		for (auto& memory : m_FramebufferMemories)
		{
			memory.reset();
		}

		for (auto& buffer : m_Framebuffers)
		{
			buffer.reset();
		}

		// Reset uniform buffers
		for (auto& buffer : m_ViewProjectionBuffers)
		{
			buffer.reset();
		}

		m_DescriptorPool.reset();

		// Cleanup
		m_Swapchain.reset();

		// Free command buffers
		for (auto& buffer : m_CommandBuffers)
		{
			buffer.reset();
		}
		
		// Reset pipeline
		m_TexturedPipeline.reset();

		// Cleanup imgui
		for (auto& buffer : m_ImGuiFramebuffers)
		{
			m_LogicalDevice->destroyFramebuffer(buffer);
		}

		m_LogicalDevice->destroyRenderPass(m_ImGuiRenderPass);

		m_LogicalDevice->freeCommandBuffers(m_ImGuiCommandPool, static_cast<uint32_t>(m_ImGuiFramebuffers.size()),m_ImGuiCommandBuffers.data());

		m_LogicalDevice->destroyDescriptorPool(m_ImGuiDescriptorPool);
		//m_LogicalDevice->destroyCommandPool(m_ImGuiCommandPool);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		
		// Now remake everything we need to
		CreateSwapchain();
		CreateGraphicsPipelines();
		CreateColorResources();
		CreateDepthResources();
		CreateFramebufferResources();
		CreateFramebuffers();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		InitaliseImgui();
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
			VEL_CORE_ERROR("An error occured in Renderer: {0}", e.what());
			VEL_CORE_ASSERT(false, e.what());
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
			VEL_CORE_ERROR("An error occured in Renderer: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create debug messenger! Error {0}", e.what());
		}

		VEL_CORE_INFO("Created debug messenger!");
	}

	// Creates a window surface. Needs to be done to influence device picking
	void Renderer::CreateSurface()
	{
		VkSurfaceKHR tmpSurface = {};

		if (glfwCreateWindowSurface(m_Instance.get(), Application::GetWindow()->GetNative(), nullptr, &tmpSurface) != VK_SUCCESS)
		{
			VEL_CORE_ERROR("Failed to create window surface!");
			VEL_CORE_ASSERT(false, "Failed to create swapchain window surface!");
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
			VEL_CORE_ERROR("Failed to find any GPUs with Vulkan support! Message:{0}", e.what());
			// Assert in debug
			VEL_CORE_ASSERT(false, "Failed to find any GPUs with Vulkan support! Message:{0}", e.what());
			// Log in release
		}

		bool foundGPU = false;
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				foundGPU = true;
				m_MSAASamples = GetMaxUsableSampleCount();
				break;
			}
		}

		if (!foundGPU)
		{
			VEL_CORE_ERROR("Failed to find a suitable GPU with Vulkan support!");
			// Assert in debug
			VEL_CORE_ASSERT(false, "Failed to find a suitable GPU with Vulkan support!");
			// Log in release
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
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

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
			VEL_CORE_ERROR("An error occured in creating the logical device: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create logical device! Error {0}", e.what());
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
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;	// We might need to copy into the swapchain if GUI is disabled
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
	void Renderer::CreateGraphicsPipelines()
	{
		// Load shaders as spv bytecode
		vk::ShaderModule vertShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/standardvert.spv");
		vk::ShaderModule fragShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/standardfrag.spv");

		vk::ShaderModule pbrVertShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/pbrvert.spv");
		vk::ShaderModule pbrFragShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/pbrfrag.spv");
		
		vk::ShaderModule skyboxVertShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/skyboxvert.spv");
		vk::ShaderModule skyboxFragShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "../Velocity/assets/shaders/skyboxfrag.spv");

		VEL_CORE_INFO("Loaded shaders!");
		
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

		// Do the same for PBR
		vk::PipelineShaderStageCreateInfo pbrVertexShaderStageInfo = {
			vk::PipelineShaderStageCreateFlags{},
			vk::ShaderStageFlagBits::eVertex,
			pbrVertShaderModule,
			"main"
		};
		vk::PipelineShaderStageCreateInfo pbrFragmentShaderStageInfo = {
			vk::PipelineShaderStageCreateFlags{},
			vk::ShaderStageFlagBits::eFragment,
			pbrFragShaderModule,
			"main"
		};

		std::array<vk::PipelineShaderStageCreateInfo, 2u> pbrShaderStages = { pbrVertexShaderStageInfo, pbrFragmentShaderStageInfo };
		
		// Do the same for skybox
		vk::PipelineShaderStageCreateInfo skyboxVertexShaderStageInfo = {
			vk::PipelineShaderStageCreateFlags{},
			vk::ShaderStageFlagBits::eVertex,
			skyboxVertShaderModule,
			"main"
		};

		vk::PipelineShaderStageCreateInfo skyboxFragmentShaderStageInfo = {
			vk::PipelineShaderStageCreateFlags{},
			vk::ShaderStageFlagBits::eFragment,
			skyboxFragShaderModule,
			"main"
		};

		std::array<vk::PipelineShaderStageCreateInfo, 2u> skyboxShaderStages = { skyboxVertexShaderStageInfo,skyboxFragmentShaderStageInfo };
		
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

		vk::PipelineRasterizationStateCreateInfo skyboxRasterizer = {
			vk::PipelineRasterizationStateCreateFlags{},
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eCounterClockwise,
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
			m_MSAASamples,
			VK_FALSE,
			1.0f,
			nullptr,
			VK_FALSE,
			VK_FALSE
		};
		
		#pragma endregion

		#pragma region DEPTH AND STENCIL

		vk::PipelineDepthStencilStateCreateInfo depthStencil = {
			vk::PipelineDepthStencilStateCreateFlags{},
			VK_TRUE,
			VK_TRUE,
			vk::CompareOp::eLess,
			VK_FALSE,
			VK_FALSE,
			{},
			{},
			0.0f,
			1.0f,
		};

		vk::PipelineDepthStencilStateCreateInfo skyboxDepthStencil = {
			vk::PipelineDepthStencilStateCreateFlags{},
			VK_TRUE,
			VK_FALSE,
			vk::CompareOp::eLessOrEqual,
			VK_FALSE,
			VK_FALSE,
			{},
			{},
			0.0f,
			1.0f,
		};
		
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

		// Setup push constant ranges
		vk::PushConstantRange modelConstantRange = {
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4)
		};

		vk::PushConstantRange textureConstantRange = {
			vk::ShaderStageFlagBits::eFragment,
			0,
			sizeof(glm::mat4) + sizeof(uint32_t) + sizeof(glm::vec3)
		};

		auto ranges = std::array<vk::PushConstantRange,2>{ modelConstantRange,textureConstantRange };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
			vk::PipelineLayoutCreateFlags{},
			0,			// Set in pipeline constructor
			nullptr,		// Set in pipeline constructor
			static_cast<uint32_t>(ranges.size()),
			ranges.data()
		};

		// Now for PBR
		vk::PushConstantRange pbrConstantRange = {
			vk::ShaderStageFlagBits::eFragment,
			0,
			sizeof(glm::mat4) + (sizeof(uint32_t) * 5) + sizeof(glm::vec3) + (sizeof(bool) * 4)
		};


		auto pbrRanges = std::array<vk::PushConstantRange, 2>{modelConstantRange, pbrConstantRange};

		vk::PipelineLayoutCreateInfo pbrLayoutInfo = {
			vk::PipelineLayoutCreateFlags{},
			0,
			nullptr,
			static_cast<uint32_t>(pbrRanges.size()),
			pbrRanges.data()
		};
		
		// Now for Skybox
		vk::PushConstantRange skyboxModelConstantRange = {
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(glm::mat4)
		};

		// Setup for skybox
		vk::PipelineLayoutCreateInfo skyboxPipelineLayoutInfo = {
			vk::PipelineLayoutCreateFlags{},
			0,	// Set in pipeline consturctor
			nullptr,
			1,
			& skyboxModelConstantRange
		};
		
		#pragma endregion

		#pragma region RENDER PASSES

		// Single colour buffer attachment (Relates to framebuffer)s
		vk::AttachmentDescription colorAttachment = {
			vk::AttachmentDescriptionFlags{},
			m_Swapchain->GetFormat(),
			m_MSAASamples,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal
		};

		// And a depth attachment
		vk::AttachmentDescription depthAttachment = {
			vk::AttachmentDescriptionFlags{},
			FindDepthFormat(),
			m_MSAASamples,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		};

		vk::AttachmentDescription colorResolveAttachment = {
			vk::AttachmentDescriptionFlags{},
			m_Swapchain->GetFormat(),
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eShaderReadOnlyOptimal
		};

		// Single subpass for now
		// Subpasses can be combined into a single renderpass for better memory usage and optimisation

		// An attachment reference must reference an attachment described with an AttachmentDescription above
		// This is done with an index
		vk::AttachmentReference colorAttachmentRef = {
			0,
			vk::ImageLayout::eColorAttachmentOptimal
		};

		vk::AttachmentReference depthAttachmentRef = {
			1,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		};

		vk::AttachmentReference colorResolveAttachmentRef = {
			2,
			vk::ImageLayout::eShaderReadOnlyOptimal
		};

		vk::SubpassDescription subpass = {
			vk::SubpassDescriptionFlags{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&colorAttachmentRef,
			&colorResolveAttachmentRef,
			&depthAttachmentRef,
			0,
			nullptr
		};

		/* We make the render pass wait on the image being acquired as there is an implicit subpass at the start
		 this transisitons the image and will be wrong if we leave it unsorted */
		vk::SubpassDependency implicitDependency = {
			VK_SUBPASS_EXTERNAL,
			0,
			vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eEarlyFragmentTests,
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
			vk::AccessFlagBits::eShaderRead,
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		};

		vk::SubpassDependency attachToShaderDependency = {
			0,
			VK_SUBPASS_EXTERNAL,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eShaderRead,
			vk::DependencyFlagBits::eByRegion
		};

		std::array<vk::SubpassDependency, 2> dependencies = { implicitDependency,attachToShaderDependency };

		std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment,depthAttachment, colorResolveAttachment };

		vk::RenderPassCreateInfo renderPassInfo = {
			vk::RenderPassCreateFlags{},
			static_cast<uint32_t>(attachments.size()),
			attachments.data(),
			1,
			&subpass,
			static_cast<uint32_t>(dependencies.size()),
			dependencies.data()
		};

		#pragma region IMGUI

		vk::AttachmentDescription imguiAttachment = {
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

		vk::AttachmentReference imguiAttachmentRef = {
			0,
			vk::ImageLayout::eColorAttachmentOptimal
		};

		vk::SubpassDescription imguiSubpass = {
			vk::SubpassDescriptionFlags{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&imguiAttachmentRef,
			nullptr,
			nullptr,
			0,
			nullptr		
		};

		vk::SubpassDependency imguiDepenency = {
			VK_SUBPASS_EXTERNAL,
			0,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlags{},
			vk::AccessFlagBits::eColorAttachmentWrite
		};

		vk::RenderPassCreateInfo imguiRenderPassInfo = {
			vk::RenderPassCreateFlags{},
			1,
			&imguiAttachment,
			1,
			&imguiSubpass,
			1,
			&imguiDepenency
		};
		
		#pragma endregion
		
		
		// We actually create this one here
		try
		{
			m_ImGuiRenderPass = m_LogicalDevice->createRenderPass(imguiRenderPassInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Failed to create imgui render pass! Error: {0}",e.what());
			VEL_CORE_ASSERT(false);
		}
		
		
		
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
			&depthStencil,
			&colorBlending,
			nullptr,				// No dynamic state yet
			nullptr,				// Pipeline Layout Supplied in pipeline constructor
			nullptr,				// Render pass supplied in pipeline constructor
			0,
			nullptr
		};

		vk::GraphicsPipelineCreateInfo pbrPipelineInfo = {
			vk::PipelineCreateFlags{},
			static_cast<uint32_t>(pbrShaderStages.size()),
			pbrShaderStages.data(),
			&vertexInputInfo,
			& inputAssembly,
			nullptr,
			& viewportState,
			& rasterizer,
			& multiSampling,
			& depthStencil,
			& colorBlending,
			nullptr,				// No dynamic state yet
			nullptr,				// Pipeline Layout Supplied in pipeline constructor
			nullptr,				// Render pass supplied in pipeline constructor
			0,
			nullptr
		};

		vk::GraphicsPipelineCreateInfo skyboxPipelineInfo = {
			vk::PipelineCreateFlags{},
			static_cast<uint32_t>(skyboxShaderStages.size()),
			skyboxShaderStages.data(),
			&vertexInputInfo,
			&inputAssembly,
			nullptr,
			&viewportState,
			&skyboxRasterizer,
			&multiSampling,
			&skyboxDepthStencil,
			&colorBlending,
			nullptr,
			nullptr,
			nullptr,
			0,
			nullptr
		};

		// Define descript set layouts
		// Binding for the View Projection uniform
		vk::DescriptorSetLayoutBinding vpLayoutBinding = {
			0,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eVertex,
			nullptr
		};

		vk::DescriptorSetLayoutBinding pointLightLayoutBinding = {
			1,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr
		};

		// Texture binding
		vk::DescriptorSetLayoutBinding textureLayoutBinding = {
			16,
			vk::DescriptorType::eCombinedImageSampler,
			128,
			vk::ShaderStageFlagBits::eFragment,
			nullptr
		};

		const std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings = { vpLayoutBinding , textureLayoutBinding , pointLightLayoutBinding };

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
			vk::DescriptorSetLayoutCreateFlags{},
			static_cast<uint32_t>(descriptorBindings.size()),
			descriptorBindings.data()
		};

		// And for skybox
		vk::DescriptorSetLayoutBinding skyboxLayoutBindingNorm = {
			1,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr
		};
		vk::DescriptorSetLayoutBinding skyboxLayoutBindingPBR = {
			2,
			vk::DescriptorType::eCombinedImageSampler,
			1,
			vk::ShaderStageFlagBits::eFragment,
			nullptr
		};


		const std::vector<vk::DescriptorSetLayoutBinding> skyboxDescriptorBindings = { vpLayoutBinding, skyboxLayoutBindingNorm };

		vk::DescriptorSetLayoutCreateInfo skyboxDescriptorSetLayoutInfo = {
			vk::DescriptorSetLayoutCreateFlags{},
			static_cast<uint32_t>(skyboxDescriptorBindings.size()),
			skyboxDescriptorBindings.data()
		};

		const std::vector<vk::DescriptorSetLayoutBinding> pbrDescriptorBindings = { vpLayoutBinding, textureLayoutBinding, skyboxLayoutBindingPBR, pointLightLayoutBinding };

		vk::DescriptorSetLayoutCreateInfo pbrDescriptorSetLayoutInfo = {
			vk::DescriptorSetLayoutCreateFlags{},
			static_cast<uint32_t>(pbrDescriptorBindings.size()),
			pbrDescriptorBindings.data()
		};

		m_TexturedPipeline = std::make_unique<Pipeline>(m_LogicalDevice, pipelineInfo, pipelineLayoutInfo, renderPassInfo, descriptorSetLayoutInfo);
		m_PBRPipeline = std::make_unique<Pipeline>(m_LogicalDevice, pbrPipelineInfo, pbrLayoutInfo, renderPassInfo, pbrDescriptorSetLayoutInfo);
		m_SkyboxPipeline = std::make_unique<Pipeline>(m_LogicalDevice, skyboxPipelineInfo, skyboxPipelineLayoutInfo, renderPassInfo, skyboxDescriptorSetLayoutInfo);

		VEL_CORE_INFO("Created graphics pipeline!");
		
		#pragma endregion
		
		// Modules hooked into the pipeline so we can delete here
		m_LogicalDevice->destroyShaderModule(vertShaderModule);
		m_LogicalDevice->destroyShaderModule(fragShaderModule);

		m_LogicalDevice->destroyShaderModule(pbrVertShaderModule);
		m_LogicalDevice->destroyShaderModule(pbrFragShaderModule);

		m_LogicalDevice->destroyShaderModule(skyboxVertShaderModule);
		m_LogicalDevice->destroyShaderModule(skyboxFragShaderModule);

	}

	// Create intermediate buffers
	void Renderer::CreateFramebufferResources()
	{
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		// Create images
		m_FramebufferImages.resize(m_Swapchain->GetImages().size());
		m_FramebufferMemories.resize(m_Swapchain->GetImages().size());
		m_FramebufferImageViews.resize(m_Swapchain->GetImages().size());

		for (size_t i = 0; i < m_Swapchain->GetImages().size(); ++i)
		{
			vk::ImageCreateInfo createInfo = {
				vk::ImageCreateFlags{},
				vk::ImageType::e2D,
				m_Swapchain->GetFormat(),
				vk::Extent3D {
					m_Swapchain->GetWidth(),
					m_Swapchain->GetHeight(),
					1
				},
				1,
				1,
				vk::SampleCountFlagBits::e1,
				vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
				vk::SharingMode::eExclusive,
				1,
				&indices.GraphicsFamily.value(),
				vk::ImageLayout::eUndefined
			};

			try
			{
				m_FramebufferImages.at(i) = m_LogicalDevice->createImageUnique(createInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the framebuffer images: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create framebuffer images! Error {0}", e.what());
			}

			// Create memory
			vk::MemoryRequirements memRequirements = m_LogicalDevice->getImageMemoryRequirements(m_FramebufferImages.at(i).get());

			vk::MemoryAllocateInfo allocInfo = {
				memRequirements.size,
				BaseBuffer::FindMemoryType(m_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
			};

			try
			{
				m_FramebufferMemories.at(i) = m_LogicalDevice->allocateMemoryUnique(allocInfo);

			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the framebuffer images memory: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create framebuffer images memory! Error {0}", e.what());
				return;
			}

			m_LogicalDevice->bindImageMemory(m_FramebufferImages.at(i).get(), m_FramebufferMemories.at(i).get(),0);

			// Create views
			vk::ImageViewCreateInfo viewInfo = {
				vk::ImageViewCreateFlags{},
				m_FramebufferImages.at(i).get(),
				vk::ImageViewType::e2D,
				m_Swapchain->GetFormat(),
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
				m_FramebufferImageViews.at(i) = m_LogicalDevice->createImageViewUnique(viewInfo);

			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the framebuffer image views: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create framebuffer image views! Error {0}", e.what());
				return;
			}
		}
	}

	// Creates framebuffers using the pipeline render pass details and the swapchain
	void Renderer::CreateFramebuffers()
	{
		// Each swapchain image needs a framebuffer
		m_Framebuffers.resize(m_Swapchain->GetImageViews().size());
		m_ImGuiFramebuffers.resize(m_Swapchain->GetImages().size());
		
		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); ++i)
		{
			std::array<vk::ImageView, 3> attachments = {
			m_ColorImageView.get(),
				m_DepthImageView.get(),
				m_FramebufferImageViews.at(i).get()
			};
			
			vk::FramebufferCreateInfo framebufferInfo = {
				vk::FramebufferCreateFlags{},
				m_TexturedPipeline->GetRenderPass().get(),
				static_cast<uint32_t>(attachments.size()),
				attachments.data(),
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

			// Update needed information to imgui
			framebufferInfo.renderPass = m_ImGuiRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &m_Swapchain->GetImageViews().at(i);

			// Create
			try
			{
				m_ImGuiFramebuffers.at(i) = m_LogicalDevice->createFramebuffer(framebufferInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occurred in creating the imgui framebuffers: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create imgui framebuffer! Error {0}", e.what());
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
			m_ImGuiCommandPool = m_LogicalDevice->createCommandPool(poolInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occurred in creating the command pool: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create command pool! Error {0}", e.what());
		}

		VEL_CORE_INFO("Created command pool!");
	}
	// Creates MSAA buffers
	void Renderer::CreateColorResources()
	{
		auto colorFormat = m_Swapchain->GetFormat();
		auto indices = FindQueueFamilies(m_PhysicalDevice);

		// Create image
		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			colorFormat,
			vk::Extent3D{
				m_Swapchain->GetWidth(),
				m_Swapchain->GetHeight(),
				1
			},
			1,
			1,
			m_MSAASamples,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
			vk::SharingMode::eExclusive,
			1,
			&indices.GraphicsFamily.value(),
			vk::ImageLayout::eUndefined
		};

		try
		{
			m_ColorImage = m_LogicalDevice->createImageUnique(imageInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Could not create msaa buffer image. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Could not msaa depth buffer image. Error: {0}", e.what());
			return;
		}

		// Create memory
		auto memRequirements = m_LogicalDevice->getImageMemoryRequirements(m_ColorImage.get());

		vk::MemoryAllocateInfo allocInfo = {
			memRequirements.size,
			BaseBuffer::FindMemoryType(m_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		try
		{
			m_ColorMemory = m_LogicalDevice->allocateMemoryUnique(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Could not create msaa buffer memory. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Could not msaa depth buffer memory. Error: {0}", e.what());
			return;
		}

		// Bind
		m_LogicalDevice->bindImageMemory(m_ColorImage.get(), m_ColorMemory.get(), 0);

		// Create imageview

		vk::ImageViewCreateInfo imageViewInfo = {
			vk::ImageViewCreateFlags{},
			m_ColorImage.get(),
			vk::ImageViewType::e2D,
			colorFormat,
			vk::ComponentMapping{},
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
			m_ColorImageView = m_LogicalDevice->createImageViewUnique(imageViewInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Could not create msaa buffer imageview. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Could not create msaa buffer imageview. Error: {0}", e.what());
		}
	}

	// Create the depth buffer 
	void Renderer::CreateDepthResources()
	{
		auto depthFormat = FindDepthFormat();
		auto indices = FindQueueFamilies(m_PhysicalDevice);

		// Create image
		vk::ImageCreateInfo imageInfo = {
			vk::ImageCreateFlags{},
			vk::ImageType::e2D,
			depthFormat,
			vk::Extent3D{
				m_Swapchain->GetWidth(),
				m_Swapchain->GetHeight(),
				1
			},
			1,
			1,
			m_MSAASamples,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::SharingMode::eExclusive,
			1,
			&indices.GraphicsFamily.value(),
			vk::ImageLayout::eUndefined
		};

		try
		{
			m_DepthImage = m_LogicalDevice->createImageUnique(imageInfo);
		}
		catch(vk::SystemError& e)
		{
			VEL_CORE_ERROR("Could not create depth buffer image. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Could not create depth buffer image. Error: {0}", e.what());
			return;
		}
		
		// Create memory
		auto memRequirements = m_LogicalDevice->getImageMemoryRequirements(m_DepthImage.get());

		vk::MemoryAllocateInfo allocInfo = {
			memRequirements.size,
			BaseBuffer::FindMemoryType(m_PhysicalDevice,memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		try
		{
			m_DepthMemory = m_LogicalDevice->allocateMemoryUnique(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Could not create depth buffer memory. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Could not create depth buffer memory. Error: {0}", e.what());
			return;
		}

		// Bind
		m_LogicalDevice->bindImageMemory(m_DepthImage.get(), m_DepthMemory.get(),0);

		// Create imageview

		vk::ImageViewCreateInfo imageViewInfo = {
			vk::ImageViewCreateFlags{},
			m_DepthImage.get(),
			vk::ImageViewType::e2D,
			depthFormat,
			vk::ComponentMapping{},
			{
				vk::ImageAspectFlagBits::eDepth,
				0,
				1,
				0,
				1
			}
		};

		try
		{
			m_DepthImageView = m_LogicalDevice->createImageViewUnique(imageViewInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("Could not create depth buffer imageview. Error: {0}", e.what());
			VEL_CORE_ASSERT(false, "Could not create depth buffer imageview. Error: {0}", e.what());
		}
		
	}

	// Creates the texture samplers
	void Renderer::CreateTextureSamplers()
	{
		// Get device properties
		auto properties = m_PhysicalDevice.getProperties();

		vk::SamplerCreateInfo samplerInfo = {
			vk::SamplerCreateFlags{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			0.0f,
			VK_TRUE,
			properties.limits.maxSamplerAnisotropy,
			VK_FALSE,
			vk::CompareOp::eAlways,
			0.0f,
			static_cast<float>(1000.0f),
			vk::BorderColor::eIntOpaqueBlack,
			VK_FALSE
		};

		try
		{
			m_TextureSampler = m_LogicalDevice->createSamplerUnique(samplerInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occurred in creating the texture samplers: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create texture sampler! Error {0}", e.what());
			return;
		}
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
			VEL_CORE_ERROR("An error occurred in creating the command buffer: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create command buffer! Error {0}", e.what());
		}

		allocInfo.commandPool = m_ImGuiCommandPool;

		try
		{
			m_ImGuiCommandBuffers = m_LogicalDevice->allocateCommandBuffers(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occurred in creating the command buffer: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create command buffer! Error {0}", e.what());
		}

		VEL_CORE_INFO("Allocated command buffers!");
	}

	// Creates the pool used to allocate descriptor sets
	void Renderer::CreateDescriptorPool()
	{
		std::array<vk::DescriptorPoolSize,2> poolSizes = {
		vk::DescriptorPoolSize{
					vk::DescriptorType::eUniformBuffer,
					static_cast<uint32_t>(512)
			},
		vk::DescriptorPoolSize{
					vk::DescriptorType::eCombinedImageSampler,
					static_cast<uint32_t>(512)
			}
		};

		vk::DescriptorPoolCreateInfo poolInfo = {
			vk::DescriptorPoolCreateFlags{},
			1024u,
			static_cast<uint32_t>(poolSizes.size()),
			poolSizes.data()
		};

		try
		{
			m_DescriptorPool = m_LogicalDevice->createDescriptorPoolUnique(poolInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occured in creating the descriptor pool: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create descriptor pool! Error {0}", e.what());
		}

		// Now create for imgui

		vk::DescriptorPoolSize imguiPoolSizes[] =
		{
			  { vk::DescriptorType::eSampler, 1000 },
			  { vk::DescriptorType::eCombinedImageSampler, 1000 },
			  { vk::DescriptorType::eSampledImage, 1000 },
			  { vk::DescriptorType::eStorageImage, 1000 },
			  { vk::DescriptorType::eUniformTexelBuffer, 1000 },
			  { vk::DescriptorType::eStorageTexelBuffer, 1000 },
			  { vk::DescriptorType::eUniformBuffer, 1000 },
			  { vk::DescriptorType::eStorageBuffer, 1000 },
			  { vk::DescriptorType::eUniformBufferDynamic, 1000 },
			  { vk::DescriptorType::eStorageBufferDynamic, 1000 },
			  { vk::DescriptorType::eInputAttachment, 1000 }
		};

		vk::DescriptorPoolCreateInfo imguiPoolInfo = {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1000 * IM_ARRAYSIZE(imguiPoolSizes),
			static_cast<uint32_t>(IM_ARRAYSIZE(imguiPoolSizes)),
			imguiPoolSizes
		};

		try
		{
			m_ImGuiDescriptorPool = m_LogicalDevice->createDescriptorPool(poolInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occured in creating the imgui descriptor pool: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create imgui descriptor pool! Error {0}", e.what());
		}
	}

	// Allocate the descriptor sets we will use accross our program.
	void Renderer::CreateDescriptorSets()
	{
		// For each pipeline we have made we need to create a descriptor set for each frame that matches its layout
		std::vector<vk::DescriptorSetLayout> layouts(m_Swapchain->GetImages().size(), m_TexturedPipeline->GetDescriptorSetLayout().get());

		std::vector<vk::DescriptorSetLayout> pbrLayouts(m_Swapchain->GetImages().size(), m_PBRPipeline->GetDescriptorSetLayout().get());

		std::vector<vk::DescriptorSetLayout> skyboxLayouts(m_Swapchain->GetImages().size(), m_SkyboxPipeline->GetDescriptorSetLayout().get());

		vk::DescriptorSetAllocateInfo allocInfo = {
			m_DescriptorPool.get(),
			static_cast<uint32_t>(m_Swapchain->GetImages().size()),
			layouts.data()
		};

		try
		{
			m_DescriptorSets = m_LogicalDevice->allocateDescriptorSets(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_INFO("Failed to create descriptor sets! Error:{0}",e.what());
			VEL_CORE_ASSERT(false, "Failed to create descriptor sets! Error:{0}", e.what());
		}

		allocInfo.pSetLayouts = pbrLayouts.data();
		try
		{
			m_PBRDescriptorSets = m_LogicalDevice->allocateDescriptorSets(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_INFO("Failed to create pbr descriptor sets! Error:{0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create pbr descriptor sets! Error:{0}", e.what());
		}
		
		allocInfo.pSetLayouts = skyboxLayouts.data();
		try
		{
			m_SkyboxDescriptorSets = m_LogicalDevice->allocateDescriptorSets(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_INFO("Failed to create skybox descriptor sets! Error:{0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to skybox create descriptor sets! Error:{0}", e.what());
		}
		

		// Configure descriptors
		
		// Load a default texture and fill texture infos
		if (m_Textures.size() == 0)
		{
			auto indices = FindQueueFamilies(m_PhysicalDevice);
			m_Textures.push_back({ "VEL_INTERNAL_DEFAULT",new Texture("../Velocity/assets/textures/default.png", m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value()) });
			m_DefaultBindingTexture = m_Textures.back().second;
			m_TextureInfos.resize(128);
			for (auto& info : m_TextureInfos)
			{
				info = {
				m_TextureSampler.get(),
				*m_DefaultBindingTexture->m_ImageView,
					vk::ImageLayout::eShaderReadOnlyOptimal,
				};
			}

			// Resize the storage of descriptor writes
			m_DescriptorWrites.resize(m_Swapchain->GetImages().size());
		}

		// Now we need to default the skybox
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		m_DefaultBindingSkybox = new Skybox("../Velocity/assets/textures/skyboxes/default", ".png", m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value());

		vk::DescriptorImageInfo pbrSkyboxBindingInfo = vk::DescriptorImageInfo{
			m_TextureSampler.get(),
			m_DefaultBindingSkybox->m_ImageView,
			vk::ImageLayout::eShaderReadOnlyOptimal
		};
		
		for (size_t i = 0; i < m_Swapchain->GetImages().size(); ++i)
		{
			m_ViewProjectionBufferInfo = vk::DescriptorBufferInfo{
				m_ViewProjectionBuffers.at(i)->Buffer.get(),
				0,
				sizeof(ViewProjection)
			};

			m_PointLightBufferInfo = vk::DescriptorBufferInfo{
				m_PointLightBuffers.at(i)->Buffer.get(),
				0,
				sizeof(PointLights)
			};
			
			
			m_DescriptorWrites.at(i) = { vk::WriteDescriptorSet{
					m_DescriptorSets.at(i),
					0,
					0,
					1,
					vk::DescriptorType::eUniformBuffer,
					nullptr,
					& m_ViewProjectionBufferInfo,
					nullptr
				},
				{
					m_DescriptorSets.at(i),
					1,
					0,
					1,
					vk::DescriptorType::eUniformBuffer,
					nullptr,
					&m_PointLightBufferInfo,
					nullptr
				},	
				{
					m_DescriptorSets.at(i),
					16,
					0,
					128,
					vk::DescriptorType::eCombinedImageSampler,
					m_TextureInfos.data(),
					nullptr,
					nullptr,
				}
			};

			m_LogicalDevice->updateDescriptorSets(static_cast<uint32_t>(m_DescriptorWrites.at(i).size()), m_DescriptorWrites.at(i).data(), 0, nullptr);

			// 
			m_PBRDescriptorWrites.resize(m_Swapchain->GetImages().size());
			m_PBRDescriptorWrites.at(i) = {
		m_DescriptorWrites.at(i).at(0),
				m_DescriptorWrites.at(i).at(1),
				// Insert the skybox descriptor
		{
					m_PBRDescriptorSets.at(i),
					2,
					0,
					1,
					vk::DescriptorType::eCombinedImageSampler,
					&pbrSkyboxBindingInfo,
					nullptr,
					nullptr
				},
		m_DescriptorWrites.at(i).at(2),
			};

			// Loop and switch descriptro set reference
			for (auto& write : m_PBRDescriptorWrites.at(i))
			{
				write.dstSet = m_PBRDescriptorSets.at(i);
			}

			m_LogicalDevice->updateDescriptorSets(static_cast<uint32_t>(m_PBRDescriptorWrites.at(i).size()), m_PBRDescriptorWrites.at(i).data(), 0, nullptr);
			
		}
		
	}

	// Creates the buffers used for uniform data
	void Renderer::CreateUniformBuffers()
	{
		// Create the buffers for camera data (view and projection matrix)
		VkDeviceSize bufferSize = sizeof(ViewProjection);

		// One copy per swapchain image (e.g. framebuffer)
		m_ViewProjectionBuffers.resize(m_Swapchain->GetImages().size());

		for (size_t i = 0; i < m_Swapchain->GetImages().size(); ++i)
		{
			m_ViewProjectionBuffers.at(i) = std::make_unique<BaseBuffer>(
				m_PhysicalDevice,
				m_LogicalDevice,
				bufferSize,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			);
		}

		bufferSize = sizeof(PointLights);

		m_PointLightBuffers.resize(m_Swapchain->GetImages().size());

		for (size_t i = 0; i < m_Swapchain->GetImages().size(); ++i)
		{
			m_PointLightBuffers.at(i) = std::make_unique<BaseBuffer>(
				m_PhysicalDevice,
				m_LogicalDevice,
				bufferSize,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			);
		}
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
				VEL_CORE_ERROR("An error occurred in creating the sync primitives: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create sync primitives! Error {0}", e.what());
			}

		}
		VEL_CORE_INFO("Created syncronisation primitives!");
	}

	// Initalises ImGui
	void Renderer::InitaliseImgui()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForVulkan(Application::GetWindow()->GetNative(), false);

		auto indices = FindQueueFamilies(m_PhysicalDevice);
		
		ImGui_ImplVulkan_InitInfo initInfo = {
			m_Instance.get(),
			m_PhysicalDevice,
			m_LogicalDevice.get(),
			indices.GraphicsFamily.value(),
			m_GraphicsQueue,
			nullptr,
			m_ImGuiDescriptorPool,
			0,
			static_cast<uint32_t>(m_Swapchain->GetImages().size()),
			static_cast<uint32_t>(m_Swapchain->GetImages().size()),
			VK_SAMPLE_COUNT_1_BIT,
			nullptr,
			[](VkResult err)
			{
				if (err != VK_SUCCESS)
				{
					std::cout << "Imgui error: " << err << "/n";
				}
			}
		};

		ImGui_ImplVulkan_Init(&initInfo, m_ImGuiRenderPass);

		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Roboto_compressed_data, Roboto_compressed_size, 15.0f);
		{
			TemporaryCommandBuffer bufferWrapper = TemporaryCommandBuffer(m_LogicalDevice, m_ImGuiCommandPool, m_GraphicsQueue);
			auto buffer = bufferWrapper.GetBuffer();
			ImGui_ImplVulkan_CreateFontsTexture(buffer);
		}

		// We need to push back a imgui texture id for the default binding texture
		// First empty it incase this is a resize
		m_TextureGUIIDs.clear();

		// Then loop all available textures at this point and push back. This will remake them on a resize and only make the first one the first time
		for (auto& texture : m_Textures)
		{
			m_TextureGUIIDs.push_back((ImTextureID)ImGui_ImplVulkan_AddTexture(m_TextureSampler.get(), texture.second->m_ImageView.get(), (VkImageLayout)texture.second->m_CurrentLayout));
		}

		m_FramebufferGUIIDs.clear();

		for (auto& view : m_FramebufferImageViews)
		{
			m_FramebufferGUIIDs.push_back((ImTextureID)ImGui_ImplVulkan_AddTexture(m_TextureSampler.get(), view.get(), static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}

		// Setup style
		#pragma region ugly style code
		auto& colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.821f, 0.821f, 0.821f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.358f, 0.358f, 0.358f, 0.88f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.11f, 0.12f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.09f, 0.08f, 0.86f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.23f, 0.29f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.56f, 0.56f, 0.58f, 0.22f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.09f, 0.08f, 0.86f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.56f, 0.56f, 0.58f, 0.22f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
		#pragma endregion
		
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
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr
		};

		try
		{
			cmdBuffer->begin(beginInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occurred in starting recording commandbuffers: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to start record commandbuffers! Error {0}", e.what());
		}

		// Now we make the draw commands
		// TODO: This will be based on the contents submitted
		
		// Set magenta clear color
		std::array<float, 4> clearColor = {
			1.0f, 0.0f, 1.0f, 1.0f
		};
		std::array<float, 4> depthClear = {
		1.0f, 0.0f, 0.0f, 0.0f
		};
		
		std::array<vk::ClearValue, 2> clearColorValues = {
			vk::ClearColorValue(clearColor),
			vk::ClearColorValue(depthClear)
		};

		// Begin render pass
		vk::RenderPassBeginInfo renderPassInfo = {
			m_TexturedPipeline->GetRenderPass().get(),
			m_Framebuffers.at(m_CurrentImage).get(),
			vk::Rect2D{{0,0},m_Swapchain->GetExtent()},
			static_cast<uint32_t>(clearColorValues.size()),
			clearColorValues.data()
		};
		cmdBuffer->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// all passes use this buffer
		m_BufferManager->Bind(cmdBuffer.get());

		// 0. Draw skybox if we have it
		if (m_ActiveScene)
		{
			if (m_ActiveScene->m_Skybox)
			{
				// Copy the descriptor writes from the main pipeline
				auto descriptorWrites = std::vector<vk::WriteDescriptorSet>{m_DescriptorWrites.at(m_CurrentImage).at(0),m_DescriptorWrites.at(m_CurrentImage).at(2)};

				// Update
				descriptorWrites.at(0).dstSet = m_SkyboxDescriptorSets.at(m_CurrentImage);
				
				descriptorWrites.at(1) = m_ActiveScene->m_Skybox->m_WriteSet;

				descriptorWrites.at(1).dstSet = m_SkyboxDescriptorSets.at(m_CurrentImage);

				descriptorWrites.at(1).dstBinding = 1;

				m_LogicalDevice->updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				
				// Now bind the pipeline
				m_SkyboxPipeline->Bind(cmdBuffer, 1, 0, m_SkyboxDescriptorSets.at(m_CurrentImage));

				auto& mesh = m_ActiveScene->m_Skybox->m_SphereMesh;

				auto& renderable = m_Renderables[mesh.MeshReference];

				auto skyboxMatrix = glm::translate(glm::mat4(1.0f), m_ActiveScene->m_SceneCamera->GetPosition()) * m_ActiveScene->m_Skybox->m_SkyboxMatrix;
				cmdBuffer->pushConstants(m_SkyboxPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), value_ptr(skyboxMatrix));
				cmdBuffer->drawIndexed(renderable.IndexCount, 1, renderable.IndexStart, renderable.VertexOffset, 0);
				
			}
		}

		// 1. Bind pipeline and buffer
		m_TexturedPipeline->Bind(cmdBuffer,1,0,m_DescriptorSets.at(m_CurrentImage));
	
		// 2. Draw static objects

		if (m_ActiveScene)
		{
			auto view = m_ActiveScene->m_Registry.view<TransformComponent, MeshComponent, TextureComponent>();

			for (auto [entity, transform, mesh, texture] : view.each())
			{
				auto& renderable = m_Renderables[mesh.MeshReference];
				
				cmdBuffer->pushConstants(m_TexturedPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::mat4), glm::value_ptr(transform.GetTransform()));
				cmdBuffer->pushConstants(m_TexturedPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(uint32_t), &texture.TextureID);
				cmdBuffer->pushConstants(m_TexturedPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) + sizeof(uint32_t), sizeof(glm::vec3), value_ptr(m_ActiveScene->m_SceneCamera->GetPosition()));
				cmdBuffer->drawIndexed(renderable.IndexCount, 1, renderable.IndexStart, renderable.VertexOffset, 0);
			}

			// When we use lights
			// TODO: TEMPORARY
			auto lights = m_ActiveScene->m_Registry.view<PointLightComponent, MeshComponent>();
			for (auto [entity, light,mesh] : lights.each())
			{
				glm::mat4 lightMatrix = translate(scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)), light.Position);
				cmdBuffer->pushConstants(m_TexturedPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::mat4), value_ptr(lightMatrix));

				auto& renderable = m_Renderables[mesh.MeshReference];
				
				// This will always point to the white default texture
				uint32_t blankTexture = 0u;
				cmdBuffer->pushConstants(m_TexturedPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), sizeof(uint32_t), &blankTexture);
				cmdBuffer->pushConstants(m_TexturedPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) + sizeof(uint32_t), sizeof(glm::vec3), value_ptr(m_ActiveScene->m_SceneCamera->GetPosition()));
				cmdBuffer->drawIndexed(renderable.IndexCount, 1, renderable.IndexStart, renderable.VertexOffset, 0);
			}
		}

		// PBR TIME
	
		if (m_ActiveScene)
		{
			// Check for skybox to see if we need to update the IBL binding
			if (m_ActiveScene->m_Skybox)
			{
				m_PBRDescriptorWrites.at(m_CurrentImage).at(2) = m_ActiveScene->m_Skybox->m_WriteSet;
				
				m_PBRDescriptorWrites.at(m_CurrentImage).at(2).dstSet = m_PBRDescriptorSets.at(m_CurrentImage);
				m_PBRDescriptorWrites.at(m_CurrentImage).at(2).dstBinding = 2;

				m_LogicalDevice->updateDescriptorSets(static_cast<uint32_t>(m_PBRDescriptorWrites.at(m_CurrentImage).size()), m_PBRDescriptorWrites.at(m_CurrentImage).data(), 0, nullptr);
			}

			
			m_PBRPipeline->Bind(cmdBuffer, 1, 0, m_PBRDescriptorSets.at(m_CurrentImage));

			auto view = m_ActiveScene->m_Registry.view<TransformComponent, MeshComponent, PBRComponent>();
			for (auto[entity, transform, mesh, pbr] : view.each())
			{
				auto& renderable = m_Renderables[mesh.MeshReference];
				
				// Transform pushed in same place
				cmdBuffer->pushConstants(m_PBRPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::mat4), glm::value_ptr(transform.GetTransform()));

				// PBR has multiple texture indexes
				cmdBuffer->pushConstants(m_PBRPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4), pbr.GetSize(), pbr.GetPointer());

				// Camera now pushed later in constant range
				cmdBuffer->pushConstants(m_PBRPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) + (sizeof(uint32_t) * 5), sizeof(glm::vec3), value_ptr(m_ActiveScene->m_SceneCamera->GetPosition()));

				// Push if skybox has been set
				bool hasSkybox[4] = { false,false,false,false };
				hasSkybox[0] = m_ActiveScene->GetSkybox() ? true : false;
				
				cmdBuffer->pushConstants(m_PBRPipeline->GetLayout().get(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) + (sizeof(uint32_t) * 5) + sizeof(glm::vec3), (sizeof(bool) * 4), &hasSkybox[0]);
				
				cmdBuffer->drawIndexed(renderable.IndexCount, 1, renderable.IndexStart, renderable.VertexOffset, 0);

			}
		}
		
		// 3. End
		cmdBuffer->endRenderPass();

		if (!m_EnableGUI)
		{
			DirectCopyToSwapchain(cmdBuffer.get());
		}

		// 4. Check
		try
		{
			cmdBuffer->end();
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occurred in recording commandbuffers: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to record commandbuffers! Error {0}", e.what());
		}
	}

	// Updates uniform buffers with scene data
	void Renderer::UpdateUniformBuffers()
	{
		ViewProjection flattenedData;
		if (m_ActiveScene)
		{
			if (!m_ActiveScene->m_SceneCamera)
			{
				VEL_CORE_WARN("You didnt set a camera!");
			}

			flattenedData = {
				m_ActiveScene->m_SceneCamera->GetViewMatrix(),
				m_ActiveScene->m_SceneCamera->GetProjectionMatrix()
			};

			void* data;

			auto result = m_LogicalDevice->mapMemory(
				m_ViewProjectionBuffers.at(m_CurrentImage)->Memory.get(),
				0,
				sizeof(ViewProjection),
				vk::MemoryMapFlags{},
				&data
			);

			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ERROR("Failed to update uniform buffers");
				VEL_CORE_ASSERT(false, "Failed to update uniform buffers");
				return;
			}

			memcpy(data, &flattenedData, sizeof(ViewProjection));

			m_LogicalDevice->unmapMemory(m_ViewProjectionBuffers.at(m_CurrentImage)->Memory.get());

			UpdatePointlightArray();

			// Now map point lights
			result = m_LogicalDevice->mapMemory(
				m_PointLightBuffers.at(m_CurrentImage)->Memory.get(),
				0,
				sizeof(PointLights),
				vk::MemoryMapFlags{},
				&data
			);

			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ERROR("Failed to update uniform buffers");
				VEL_CORE_ASSERT(false, "Failed to update uniform buffers");
				return;
			}

			memcpy(data, &m_Lights, sizeof(PointLights));

			m_LogicalDevice->unmapMemory(m_PointLightBuffers.at(m_CurrentImage)->Memory.get());
		}

		
	}

	// Takes all ImGui commands sent and records the buffers for them
	void Renderer::RecordImGuiCommandBuffers()
	{
		// Start a command buffer
		vk::CommandBufferBeginInfo cmdInfo = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		};
		
		m_ImGuiCommandBuffers.at(m_CurrentImage).begin(cmdInfo);

		vk::ClearValue clearColor = { vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}} };
		vk::RenderPassBeginInfo renderPassInfo = {
			m_ImGuiRenderPass,
			m_ImGuiFramebuffers.at(m_CurrentImage),
			vk::Rect2D{{0,0},m_Swapchain->GetExtent()},
			 1,
			&clearColor
		};

		m_ImGuiCommandBuffers.at(m_CurrentImage).beginRenderPass(renderPassInfo,vk::SubpassContents::eInline);

		// Add imgui draw calls
		if (ImGui::GetDrawData() != nullptr)
		{
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_ImGuiCommandBuffers.at(m_CurrentImage));
		}

		m_ImGuiCommandBuffers.at(m_CurrentImage).endRenderPass();
		
		m_ImGuiCommandBuffers.at(m_CurrentImage).end();
	}

	// Copies the final result of the first render pass and copies it into the swapchain directly if GUI is disabled
	void Renderer::DirectCopyToSwapchain(vk::CommandBuffer& cmdBuffer)
	{
		// Transition framebuffer from shader read to transfer src
		Texture::TransitionImageLayout(
			cmdBuffer,
			m_FramebufferImages.at(m_CurrentImage).get(),
			m_Swapchain->GetFormat(),
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::ImageLayout::eTransferSrcOptimal,
			1,
			1
		);

		// Transition swapchain from undefined to transfer dst
		Texture::TransitionImageLayout(
			cmdBuffer,
			m_Swapchain->GetImages().at(m_CurrentImage),
			m_Swapchain->GetFormat(),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			1,
			1
		);
		
		vk::ImageCopy copyRegion = {
			vk::ImageSubresourceLayers {
				vk::ImageAspectFlagBits::eColor,
				0,
				0,
				1
			},
			vk::Offset3D {},
			vk::ImageSubresourceLayers {
				vk::ImageAspectFlagBits::eColor,
				0,
				0,
				1,
			},
			vk::Offset3D{},
			vk::Extent3D {
				m_Swapchain->GetWidth(),
				m_Swapchain->GetHeight(),
				1
			}
		};
		
		cmdBuffer.copyImage(
			m_FramebufferImages.at(m_CurrentImage).get(),
			vk::ImageLayout::eTransferSrcOptimal,
			m_Swapchain->GetImages().at(m_CurrentImage),
			vk::ImageLayout::eTransferDstOptimal,
			1,
			&copyRegion
		);

		// Transition swapchain back to present
		
		Texture::TransitionImageLayout(
			cmdBuffer,
			m_Swapchain->GetImages().at(m_CurrentImage),
			m_Swapchain->GetFormat(),
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			1,
			1
		);
		
		
	}

	// Draws viewport image into an imgui window
	void Renderer::DrawViewport()
	{
		// Get a reference to the imgui IO
		auto& io = ImGui::GetIO();
		
		// Setup style vars for this window
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));   // Reduced padding for nicer fill on viewport

		ImGui::Begin("Viewport");
		
		
		// Get the size of the image
		auto sourceSize = ImVec2(m_Swapchain->GetWidthF(), m_Swapchain->GetHeightF());
		// Get the size of the window
		auto dstSize = ImGui::GetWindowContentRegionMax();

		// Work out the scale between the two
		float imagescale = glm::min(dstSize.x / sourceSize.y, dstSize.y / sourceSize.y);
		
		// Scale the source size
		ImVec2 finalSize = { sourceSize.x * imagescale, sourceSize.y * imagescale };

		// Calculate and set central pos in window
		ImVec2 halfCursorPos = { (ImGui::GetWindowContentRegionMax().x - finalSize.x) * 0.5f,(ImGui::GetWindowContentRegionMax().y - finalSize.y) * 0.5f };
		ImGui::SetCursorPos(halfCursorPos);

		// Draw image at this point and size
		ImGui::Image(m_FramebufferGUIIDs.at(m_CurrentImage), finalSize);

		// Set the size of this window in the IO struct for camera to pickup on
		io.ViewportImageSize = finalSize;

		// If they want a gizmo
		if (m_GizmoEntity)
		{
			// Point Lights dont have specific components for transform
			if (m_GizmoEntity->HasComponent<TransformComponent>())
			{
				// Gizmos
				// Set parameters
				ImGuizmo::SetOrthographic(false);	// Normal view
				ImGuizmo::SetDrawlist();			// Marks window for use

				// Set viewport size
				const auto windowWidth = static_cast<float>(ImGui::GetWindowWidth());
				const auto windowHeight = static_cast<float>(ImGui::GetWindowHeight());
				ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

				// Get camera parameters
				auto camera = m_ActiveScene->GetCamera();
				glm::mat4 cameraView = camera->GetViewMatrix();
				glm::mat4 cameraProjection = camera->GetProjectionMatrix();
				cameraProjection[1][1] *= -1.0f;

				// Entity
				auto& tc = m_GizmoEntity->GetComponent<TransformComponent>();
				glm::mat4 transform = tc.GetTransform();

				// Draw a gizmo
				Manipulate(value_ptr(cameraView), value_ptr(cameraProjection), m_GizmoOperation, m_GizmoMode,value_ptr(transform));

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, scale, skew;
					glm::quat rotation;
					glm::vec4 perspective;
					decompose(transform, scale, rotation, translation, skew, perspective);

					glm::vec3 eulerRotation = 0.0f - eulerAngles(rotation);
					glm::vec3 deltaRotation = eulerRotation - tc.Rotation;
					
					tc.Translation = translation;
					tc.Rotation += deltaRotation;
					tc.Scale = scale;
					
					
				} 
			}
			// Point light components only store position for effiency on vulkan code
			if (m_GizmoEntity->HasComponent<PointLightComponent>())
			{
				// Set parameters
				ImGuizmo::SetOrthographic(false);	// Normal view
				ImGuizmo::SetDrawlist();			// Marks window for use

				// Set viewport size
				const auto windowWidth = static_cast<float>(ImGui::GetWindowWidth());
				const auto windowHeight = static_cast<float>(ImGui::GetWindowHeight());
				ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

				// Get camera parameters
				auto camera = m_ActiveScene->GetCamera();
				glm::mat4 cameraView = camera->GetViewMatrix();
				glm::mat4 cameraProjection = camera->GetProjectionMatrix();
				cameraProjection[1][1] *= -1.0f;

				// Construct temp transform
				glm::mat4 tempTransform = glm::translate(glm::mat4(1.0f), m_GizmoEntity->GetComponent<PointLightComponent>().Position);

				// Draw gizmo ignoring the user operation and forcing translate
				Manipulate(value_ptr(cameraView), value_ptr(cameraProjection), ImGuizmo::OPERATION::TRANSLATE, m_GizmoMode, value_ptr(tempTransform));

				if (ImGuizmo::IsUsing())
				{
					m_GizmoEntity->GetComponent<PointLightComponent>().Position = tempTransform[3];
				}
				
			}
		}
		
		// End the window
		ImGui::End();
		
		ImGui::PopStyleVar();
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

		auto features = device.getFeatures();

		bool hasSwapchain = false;
		if (hasExtensions)
		{
			auto swapChainSupport = QuerySwapchainSupport(device);
			hasSwapchain = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
		}

		return hasIndices && hasExtensions && hasSwapchain && features.samplerAnisotropy && features.shaderSampledImageArrayDynamicIndexing;
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

	// Find a depth format our device supports
	vk::Format Renderer::FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
	{
		// For each format
		for (auto format : candidates)
		{
			auto props = m_PhysicalDevice.getFormatProperties(format);

			if ((tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) ||
				(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features))
			{
				return format;
			}

		}

		VEL_CORE_ERROR("Failed to find a suitable depth format!");
		VEL_CORE_ASSERT(false, "Failed to find a suitable depth format!");
		return vk::Format::eR8Srgb;	// Return a dumb value to break it. The error is logged
		
	}

	#pragma endregion

	#pragma region ECS CALLBACKS

	void Renderer::UpdatePointlightArray()
	{
		// Sanity check, but this will never be false if this gets called
		if (m_ActiveScene)
		{
			auto view = m_ActiveScene->m_Registry.view<PointLightComponent>();
			
			m_Lights.ActiveLightCount = 0u;
			for (auto [entity,pointLight] : view.each())
			{
				// A transform component overrides point lights internal position
				// TODO: MAKE THIS MORE APPARANT
				if (m_ActiveScene->m_Registry.has<TransformComponent>(entity))
				{
					auto& transform = m_ActiveScene->m_Registry.get<TransformComponent>(entity);

					m_Lights.Lights[m_Lights.ActiveLightCount].Position = transform.Translation;
				}

				m_Lights.Lights[m_Lights.ActiveLightCount] = pointLight;
				m_Lights.ActiveLightCount += 1u;
			}
		}
	}

	#pragma endregion

	void Renderer::CreateTexture(std::unique_ptr<stbi_uc> pixels, int width, int height, const std::string& referenceName)
	{
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		m_Textures.push_back({referenceName,new Texture(std::move(pixels),width,height,m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value()) });
		auto newIndex = static_cast<uint32_t>(m_Textures.size()) - 1u;
		// Update the texture info
		m_TextureInfos.at(newIndex).imageView = m_Textures.back().second->m_ImageView.get();

		m_LogicalDevice->waitIdle();

		for (auto writes : m_DescriptorWrites)
		{
			m_LogicalDevice->updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

		}

		m_TextureGUIIDs.push_back((ImTextureID)ImGui_ImplVulkan_AddTexture(m_TextureSampler.get(), m_Textures.back().second->m_ImageView.get(), (VkImageLayout)m_Textures.back().second->m_CurrentLayout));


	}

	Skybox* Renderer::CreateSkybox(std::array<std::unique_ptr<stbi_uc>, 6>& pixels, int width, int height)
	{
		auto indices = FindQueueFamilies(m_PhysicalDevice);
		return new Skybox(pixels, width, height, m_LogicalDevice, m_PhysicalDevice, m_CommandPool.get(), indices.GraphicsFamily.value());
		
	}

	
	
	// Destroys all vulkan data
	Renderer::~Renderer()
	{
		// Need to force this to happen before the other variables go out of scope
		m_Swapchain.reset();

		// Cleanup imgui
		for (auto& buffer : m_ImGuiFramebuffers)
		{
			m_LogicalDevice->destroyFramebuffer(buffer);
		}

		m_LogicalDevice->destroyRenderPass(m_ImGuiRenderPass);

		m_LogicalDevice->freeCommandBuffers(m_ImGuiCommandPool, static_cast<uint32_t>(m_ImGuiFramebuffers.size()), m_ImGuiCommandBuffers.data());
		m_LogicalDevice->destroyCommandPool(m_ImGuiCommandPool);
		m_LogicalDevice->destroyDescriptorPool(m_ImGuiDescriptorPool);

		for (auto& texture : m_Textures)
		{
			delete texture.second;
		}
		
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	
}
