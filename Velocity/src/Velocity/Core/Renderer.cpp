#include "velpch.h"

#include "Renderer.hpp"
#include <Velocity/Core/Log.hpp>

namespace Velocity
{
	std::unique_ptr<Renderer> Renderer::s_Renderer = nullptr;

	// Initalises Vulkan so it is ready to render
	Renderer::Renderer()
	{
		CreateInstance();
		SetupDebugMessenger();
		PickPhysicalDevice();
	}

	// -- INITALISATION FUNCTIONS -- //

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
			m_Instance = vk::createInstance(createInfo, nullptr);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, e.what());
			VEL_CORE_ERROR("An error occured in Renderer: {0}", e.what());
		}

		m_InstanceLoader = vk::DispatchLoaderDynamic(m_Instance, vkGetInstanceProcAddr);

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
			m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(createInfo,nullptr, m_InstanceLoader);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create debug messenger! Error {0}",e.what());
			VEL_CORE_ERROR("An error occured in Renderer: {0}", e.what());
		}
	}

	// Selects a physical GPU from the PC
	void Renderer::PickPhysicalDevice()
	{
		// Prepare list of devices
		std::vector<vk::PhysicalDevice> devices;
		// Get list
		try
		{
			devices = m_Instance.enumeratePhysicalDevices();
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

		VEL_CORE_INFO("Selected GPU {0}", m_PhysicalDevice);


	}

	// -- INITALISATION FUNCTIONS -- //

	// -- HELPER FUNCTIONS -- //

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
		return true;
	}

	// -- HELPER FUNCTIONS -- //

	// Destroys all vulkan data
	Renderer::~Renderer()
	{
		// If we are in debug mode and used validation layers
		if (ENABLE_VALIDATION_LAYERS)
		{
			m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_InstanceLoader);
		}

		vkDestroyInstance(m_Instance, nullptr);
	}
}