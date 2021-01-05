#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <optional>

#include <Velocity/Core/Events/ApplicationEvent.hpp>

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

		// Submits a renderer command to be done
		// TODO: Make this actually work as it is. Im placeholding the record logic in here so it reads right from the application side
		void Submit();

		// Calls RecordCommandBuffers to flush all Submitted commands
		// Then syncrohnises and presents a frame
		// Called by application in the run loop
		void Render();

		// Called by app when resize occurs
		void OnWindowResize();

		// Call as application exits
		void Finalise() { m_LogicalDevice->waitIdle(); } ;
		
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

		static const uint32_t MAX_FRAMES_IN_FLIGHT = 2u;

		#pragma endregion
		
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

		// Contains all syncronization primitives needed for this renderer
		struct Syncronizer
		{
			std::array<vk::UniqueSemaphore, MAX_FRAMES_IN_FLIGHT>	ImageAvailable;
			std::array<vk::UniqueSemaphore, MAX_FRAMES_IN_FLIGHT>	RenderFinished;
			std::array<vk::UniqueFence, MAX_FRAMES_IN_FLIGHT>		InFlightFences;
			std::vector<vk::Fence>									ImagesInFlight;
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

		// Creates framebuffers using the pipeline render pass details and the swapchain
		void CreateFramebuffers();

		// Creates the pool we will use to create all command buffers
		void CreateCommandPool();

		// Allocates one command buffer per framebuffer
		void CreateCommandBuffers();

		// Creates all required sync primitives
		void CreateSyncronizer();

		
		#pragma endregion

		#pragma region RENDERING FUNCTIONS

		// Takes all commands sent through Renderer::Submit and records the buffers for them
		void RecordCommandBuffers();
		
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

		void RecreateSwapchain();

		#pragma endregion

		#pragma region MEMBER VARIABLES
		// Loads extenstion functions dynamically at runtime
		vk::DispatchLoaderDynamic				m_InstanceLoader;

		// Instance handle
		vk::UniqueInstance						m_Instance;

		// Window surface. Passed into swapchain
		vk::UniqueSurfaceKHR					m_Surface;

		// Physical GPU handle
		vk::PhysicalDevice						m_PhysicalDevice;

		// Logical Vulkan Device handle ^ interfaces with PhysicalDevice
		vk::UniqueDevice						m_LogicalDevice;

		// Need to store the handle for the graphics queue
		vk::Queue								m_GraphicsQueue;
		
		// Also need to store present queue
		vk::Queue								m_PresentQueue;

		// Debug messaging callback
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT,vk::DispatchLoaderDynamic>	m_DebugMessenger;

		// Swapchain class
		std::unique_ptr<Swapchain>				m_Swapchain;

		// Pipeline class - We only need one to start
		std::unique_ptr<Pipeline>				m_GraphicsPipeline;

		// Collection of framebuffers for the swapchain
		// TODO: Check if this can be made a part of the swapchain class
		std::vector<vk::UniqueFramebuffer>		m_Framebuffers;

		// Command pools which are used to allocate command buffers
		// TODO: Check if these need to be moved to swapchain aswell
		vk::UniqueCommandPool					m_CommandPool;

		// Command buffers - one per swapchain
		std::vector<vk::UniqueCommandBuffer>	m_CommandBuffers;

		// Contains all sync primitives needed
		Syncronizer								m_Syncronizer;
		size_t									m_CurrentFrame = 0u;
		uint32_t								m_CurrentImage = 0u;
		
		#pragma endregion


	};

}
