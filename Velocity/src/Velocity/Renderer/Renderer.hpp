#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <optional>

#include <Velocity/Core/Events/ApplicationEvent.hpp>

#include "BufferManager.hpp"

namespace Velocity {

	// Forward declaration
	// Safer in a class as big as this
	class Window;
	class Swapchain;
	class Pipeline;
	class Shader;
	class BufferManager;
	class Texture;

	// This is the BIG class. Contains all vulkan related code
	class Renderer
	{
	public:
		Renderer();

		virtual ~Renderer();

		// Calls RecordCommandBuffers to flush all Submitted commands
		// Then syncrohnises and presents a frame
		// Called by application in the run loop
		void Render();
		
		// Called by app when resize occurs
		void OnWindowResize();

		// Call as application exits
		void Finalise() { m_LogicalDevice->waitIdle(); } ;

		#pragma region USER API

		// This is called when you want to start the rendering of a scene!
		void BeginScene(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

		// This is called to end the rendering of a scene
		void EndScene();

		// Loads the given mesh into a renderable object
		// Pass this to Renderer::Submit
		BufferManager::Renderable LoadMesh(std::vector<Vertex>& verts,std::vector<uint32_t>& indices)
		{
			return m_BufferManager->AddMesh(verts,indices);
		}

		// Submits a renderer command to be done.

		// This allows you to add an object that will never move. Add it once and it will always be drawn
		void AddStatic(BufferManager::Renderable object,uint32_t textureID, const glm::mat4& modelMatrix);

		// This allows you to add an object that may change from frame to frame. YOU MUST CALL THIS EACH FRAME WITH THINGS YOU WANT TO DRAW
		void DrawDynamic(BufferManager::Renderable object, uint32_t textureID, const glm::mat4& modelMatrix);

		// Create assets

		// TODO: Check ownership here
		// Returns a new texture. The index is what is passed into the render commands
		uint32_t CreateTexture(const std::string& filepath);

		std::unique_ptr<Texture>* GetTextureByID(uint32_t textureID)
		{
			if (textureID > static_cast<uint32_t>(m_Textures.size()) - 1u)
			{
				VEL_CORE_ASSERT(false, "Tried to get a texture that hasnt been loaded!");
				VEL_CORE_ERROR("Tried to get a texture that hasnt been loaded!. Your texture is likely null!");
				return nullptr;
			}
			else
			{
				return &m_Textures.at(textureID);
			}
		}
		

		#pragma endregion 
		
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

		// Contains a group of renderering data
		struct RenderBatchedObject
		{
			BufferManager::Renderable	m_Object;
			glm::mat4					m_Transform;
			uint32_t					m_Texture;
		};
		
		// Contains the data for a scene
		struct SceneData
		{
			glm::mat4 m_ViewMatrix;
			glm::mat4 m_ProjectionMatrix;

			// These objects CANNOT be modified after you push them
			// TODO: Make it so you can at least remove them
			bool									m_StaticSorted = false;
			std::vector<RenderBatchedObject>		m_StaticScene;

			// These objects are flushed each frame. So they must be pushed per frame
			bool									m_DynamicSorted = false;
			std::vector<RenderBatchedObject>		m_DynamicScene;
		};

		
		// Matches the UBO used to pass over view & projection data per scene
		struct ViewProjection
		{
			glm::mat4 view;
			glm::mat4 proj;
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

		// Create the depth buffer 
		void CreateDepthResources();

		// Creates the pool we will use to create all command buffers
		void CreateCommandPool();

		// Creates the texture samplers
		void CreateTextureSamplers();

		// Creates the vertex and index buffers we need
		void CreateBufferManager();

		// Creates the buffers used for uniform data
		void CreateUniformBuffers();

		// Creates the pool used to allocate descriptor sets
		void CreateDescriptorPool();

		// Allocate the descriptor sets we will use accross our program.
		void CreateDescriptorSets();
		
		// Allocates one command buffer per framebuffer
		void CreateCommandBuffers();

		// Creates all required sync primitives
		void CreateSyncronizer();

		
		#pragma endregion

		#pragma region RENDERING FUNCTIONS

		// Takes all commands sent through Renderer::Submit and records the buffers for them
		void RecordCommandBuffers();

		// Updates uniform buffers with scene data
		void UpdateUniformBuffers();
		
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

		// Find a format from canditates that supports the tiling and features needed
		vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

		// Find depth format
		vk::Format FindDepthFormat()
		{
			return FindSupportedFormat(
				{ vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint },
				vk::ImageTiling::eOptimal,
				vk::FormatFeatureFlagBits::eDepthStencilAttachment
			);
		}

		// Check for stencil
		bool HasStencilComponent(vk::Format format)
		{
			return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
		}
		
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
		std::unique_ptr<Pipeline>				m_TexturedPipeline;

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

		// Contains the vertex and index buffers and provides interface to load into them
		std::unique_ptr<BufferManager>			m_BufferManager;

		// This structure is flushed every frame
		// TODO: UNLESS renderer::setstatic is called ?
		SceneData									m_SceneData;

		// One buffer with a copy of the view projection per frame. Because we may need to update it for a new frame whilst other is in flight
		std::vector<std::unique_ptr<BaseBuffer>>	m_ViewProjectionBuffers;

		// Descriptor pools which are used to allocate descriptor sets
		vk::UniqueDescriptorPool					m_DescriptorPool;

		// This might seem like it should go in Pipeline.
		// HOWEVER descriptor sets are not unique to a single pipeline, as long as the layout is compatitible

		// Store this as it will be the same for any pipeline we make 99% of time
		vk::DescriptorBufferInfo							m_ViewProjectionBufferInfo;

		// Also need to cache the writes for when we update textures
		std::vector<std::array<vk::WriteDescriptorSet,2>>	m_DescriptorWrites;

		// Store the actualy sets
		std::vector<vk::DescriptorSet>						m_DescriptorSets;

		// Used to sample textures passed in by the user on draw commands
		vk::UniqueSampler					m_TextureSampler;

		// Limitation of wanting to stick to no extensions, I need a texture to bind to by default
		std::unique_ptr<Texture>*			m_DefaultBindingTexture;

		// List of textures loaded by the user
		std::vector<std::unique_ptr<Texture>>	m_Textures;
		std::vector<vk::DescriptorImageInfo>	m_TextureInfos;

		// Depth Buffer

		vk::UniqueImage							m_DepthImage;
		vk::UniqueDeviceMemory					m_DepthMemory;
		vk::UniqueImageView						m_DepthImageView;
		
		#pragma endregion


	};

}
