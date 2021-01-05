#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <optional>

namespace Velocity {

	// Forward declaration
	// Safer in a class as big as this
	class Window;
	class Swapchain;
	class Pipeline;
	class Shader;

	// This is the BIG class. Contains all vulkan related code
	class Renderer
	{
	public:
		Renderer();

		virtual ~Renderer();
		
		static std::shared_ptr<Renderer>& GetRenderer()
		{
			if (!s_Renderer)
			{
				s_Renderer = std::make_shared<Renderer>();
			}
			return s_Renderer;
		}

	private:
		// Renderer is a singleton
		static std::shared_ptr<Renderer> s_Renderer;

		#pragma region TYPEDEFS AND STRUCTS

		struct QueueFamilyIndices
		{
			std::optional<uint32_t> GraphicsFamily;
			std::optional<uint32_t> PresentFamily;

			bool IsComplete()
			{
				return GraphicsFamily.has_value() && PresentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR			Capabilities;
			std::vector<vk::SurfaceFormatKHR>	Formats;
			std::vector<vk::PresentModeKHR>		PresentModes;
		};
		
		#pragma endregion 

		#pragma region INITALISATION FUNCTIONS
		// Creates the instance to Vulkan. Basically initalises Vulkan itself.
		void CreateInstance();

		// Sets up the debug messenger callback
		void SetupDebugMessenger();

		// Creates a window surface. Needs to be done to influence device picking
		void CreateSurface();

		// Selects a physical GPU from the PC
		void PickPhysicalDevice();

		// Creates a Vulkan logical device to interface with the physical device
		void CreateLogicalDevice();

		// Creates the swapchain (creates chain, gets and makes images & views)
		void CreateSwapchain();

		// Chonky function that creates a full pipeline
		void CreateGraphicsPipeline();

		#pragma endregion
		
		#pragma region HELPER FUNCTIONS
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

		// Checks if a device has required extensions
		bool CheckDeviceExtensionsSupport(vk::PhysicalDevice device);

		// Checks if a device can support our swapchain requirements
		SwapChainSupportDetails QuerySwapchainSupport(vk::PhysicalDevice device);

		// Finds the required queue families for the device
		QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device);

		#pragma endregion

		#pragma region MEMBER VARIABLES
		// Loads extenstion functions dynamically at runtime
		vk::DispatchLoaderDynamic			m_InstanceLoader;

		// Instance handle
		vk::UniqueInstance					m_Instance;

		// Window surface. Passed into swapchain
		vk::UniqueSurfaceKHR				m_Surface;

		// Physical GPU handle
		vk::PhysicalDevice					m_PhysicalDevice;

		// Logical Vulkan Device handle ^ interfaces with PhysicalDevice
		vk::UniqueDevice					m_LogicalDevice;

		// Need to store the handle for the graphics queue
		vk::Queue							m_GraphicsQueue;
		
		// Also need to store present queue
		vk::Queue							m_PresentQueue;

		// Debug messaging callback
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT,vk::DispatchLoaderDynamic>	m_DebugMessenger;

		// Swapchain class
		std::unique_ptr<Swapchain>			m_Swapchain;
		
		#pragma endregion

		#pragma region CONSTANT DATA
		const std::array<const char*, 1> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		const std::array<const char*, 1> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		#ifdef VEL_DEBUG
		const bool ENABLE_VALIDATION_LAYERS = true;
		#else
		const bool ENABLE_VALIDATION_LAYERS = false;
		#endif
		
		#pragma endregion 

	};

}
