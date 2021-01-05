#include "velpch.h"

#include "Renderer.hpp"
#include <Velocity/Core/Log.hpp>

#include <Velocity/Core/Window.hpp>
#include <Velocity/Core/Application.hpp>

#include <Velocity/Renderer/Swapchain.hpp>
#include <Velocity/Renderer/Shader.hpp>

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
		vk::ShaderModule vertShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "assets/shaders/vert.spv");
		vk::ShaderModule fragShaderModule = Shader::CreateShaderModule(m_LogicalDevice, "assets/shaders/frag.spv");



		// Modules hooked into the pipeline so we can delete here
		m_LogicalDevice->destroyShaderModule(vertShaderModule);
		m_LogicalDevice->destroyShaderModule(fragShaderModule);

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
		}

		return VK_FALSE;
	}

	// Fills in the structure with the data for a debug messenger
	void Renderer::PopulateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
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