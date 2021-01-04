#pragma once

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

namespace Velocity {

	// This is the BIG class. Contains all vulkan related code
	class Renderer
	{
	public:
		Renderer();

		virtual ~Renderer();

		static std::unique_ptr<Renderer>* GetRenderer()
		{
			if (!s_Renderer)
			{
				s_Renderer = std::make_unique<Renderer>();
			}
			return &s_Renderer;
		}

	private:
		// Renderer is a singleton
		static std::unique_ptr<Renderer> s_Renderer;

		// -- INITALISATION FUNCTIONS -- //

		// Creates the instance to Vulkan. Basically initalises Vulkan itself.
		void CreateInstance();

		// Sets up the debug messenger callback
		void SetupDebugMessenger();

		// Selects a physical GPU from the PC
		void PickPhysicalDevice();

		// -- INITALISATION FUNCTIONS -- //

		// -- HELPER FUNCTIONS -- //

		// Checks for required validation layers
		bool CheckValidationLayerSupport();

		// Returns a list of required extensions dependant on if Validation Layers are needed
		std::vector<const char*> GetRequiredExtensions();

		// This is fucking horrible. But its a way to hook into debug events and send them through Velocity
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		// Fills in the structure with the data for a debug messenger
		void PopulateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

		// Checks if a physical device is suitable
		bool IsDeviceSuitable(vk::PhysicalDevice device);

		// -- HELPER FUNCTIONS -- //

		// -- MEMBER VARIABLES -- //

		// Loads extenstion functions dynamically at runtime
		vk::DispatchLoaderDynamic	m_InstanceLoader;

		// Instance handle
		vk::Instance				m_Instance;

		// Physical GPU handle
		vk::PhysicalDevice			m_PhysicalDevice;

		// Debug messaging callback
		vk::DebugUtilsMessengerEXT	m_DebugMessenger;

		// -- MEMBER VARIABLES -- //

		// -- CONSTANT DATA -- //

		const std::array<const char*, 1> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef VEL_DEBUG
		const bool ENABLE_VALIDATION_LAYERS = true;
#else
		const bool ENABLE_VALIDATION_LAYERS = false;
#endif

		// -- CONSTANT DATA -- //

	};

}