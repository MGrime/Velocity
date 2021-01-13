#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <optional>
#include <backends/imgui_impl_vulkan.h>

#include <Velocity/Core/Events/ApplicationEvent.hpp>

#include "BufferManager.hpp"
#include "imgui.h"
#include "Texture.hpp"


namespace Velocity {

	// Forward declaration
	// Safer in a class as big as this
	class Window;
	class Swapchain;
	class Pipeline;
	class Shader;
	class BufferManager;
	class Texture;
	class Scene;
	class Skybox;

	// This is the BIG class. Contains all vulkan related code
	class Renderer
	{
		friend class Scene;						// Scene needs to update some data when components are added/removed
		friend class ImGuiLayer;				// Gui needs to call a viewport function
		friend class DefaultCameraController;	// Needs to check gui state
		friend class Application;				// Same as camera controller
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
		void BeginScene(Scene* scene);

		// This is called to end the rendering of a scene
		void EndScene();

		// Loads the given raw mesh into a renderable object
		void LoadMesh(std::vector<Vertex>& verts,std::vector<uint32_t>& indices, const std::string& referenceName)
		{
			m_Renderables.insert({ referenceName,m_BufferManager->AddMesh(verts, indices) });
		}

		// Loads the given mesh file into a renderable object
		// Pass this to Renderer::Submit
		void LoadMesh(const std::string& filepath, const std::string& referenceName)
		{
			m_Renderables.insert({ referenceName,m_BufferManager->AddMesh(filepath) });
		}

		// Gets the list of meshes
		const std::unordered_map<std::string, BufferManager::MeshIndexer>& GetMeshList()
		{
			return m_Renderables;
		}

		// Texture functions

		// Returns a new texture. The index is what is passed into the render commands
		uint32_t CreateTexture(const std::string& filepath, const std::string& referenceName);

		// Returns a material component
		PBRComponent CreatePBRMaterial(const std::string& basefilepath, const std::string& extension, const std::string& referenceName);

		// Returns a skybox
		Skybox* CreateSkybox(const std::string& baseFilepath, const std::string& extension);

		// Gets a texture by the reference name you gave when loading it
		uint32_t GetTextureByReference(const std::string& texture)
		{
			// TODO: IMPROVE SEARCH
			for (size_t i = 0; i < m_Textures.size(); ++i)
			{
				if (m_Textures.at(i).first.compare(texture) == 0)
				{
					return static_cast<uint32_t>(i);
				}
			}

			VEL_CORE_ERROR("Tried to get a texture that hasnt been loaded!. Returning the default texture (white square)");
			VEL_CORE_ASSERT(false, "Tried to get a texture that hasnt been loaded!");
			return 0;
		}

		// Draws the given texture to the screen (calls ImGui::Image())
		void DrawTextureToGUI(const std::string& textureReference, const ImVec2& size)
		{
			// TODO: IMPROVE SEARCH
			uint32_t textureID = 0u;
			for (size_t i = 0; i < m_Textures.size(); ++i)
			{
				if (m_Textures.at(i).first.compare(textureReference) == 0)
				{
					textureID = static_cast<uint32_t>(i);
					break;
				}
			}

			auto& textureGUIID = m_TextureGUIIDs.at(textureID);

			ImGui::Image(
				textureGUIID,
				size
			);
		}

		// Gets the list of textures
		const std::vector<std::pair<std::string,Texture*>>& GetTexturesList()
		{
			return m_Textures;
		}

		// Switch the whole imgui pipeline on or off
		void ToggleGUI()
		{
			m_EnableGUI = !m_EnableGUI;
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

		static const uint32_t MAX_LIGHTS = 128;

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
		
		// Matches the UBO used to pass over view & projection data per scene
		struct ViewProjection
		{
			glm::mat4 view;
			glm::mat4 proj;
		};

		// Matches the UBO used to pass over light data per scene
		struct PointLights
		{
			uint32_t												ActiveLightCount;
			alignas(16) std::array<PointLightComponent, MAX_LIGHTS> Lights;
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
		void CreateGraphicsPipelines();

		// Create intermediate buffers
		void CreateFramebufferResources();
		
		// Creates framebuffers using the pipeline render pass details and the swapchain
		void CreateFramebuffers();

		// Creates MSAA buffers
		void CreateColorResources();

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

		// Initalises ImGui
		void InitaliseImgui();
		
		#pragma endregion

		#pragma region RENDERING FUNCTIONS

		// Takes all commands sent through Renderer::Submit and records the buffers for them
		void RecordCommandBuffers();

		// Takes all ImGui commands sent and records the buffers for them
		void RecordImGuiCommandBuffers();

		// Copies the final result of the first render pass and copies it into the swapchain directly if GUI is disabled
		void DirectCopyToSwapchain(vk::CommandBuffer& cmdBuffer);

		// Updates uniform buffers with scene data
		void UpdateUniformBuffers();

		// Draws viewport image into an imgui window
		void DrawViewport();
		
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

		// Get the maximum supported msaa samples
		vk::SampleCountFlagBits GetMaxUsableSampleCount()
		{
			vk::PhysicalDeviceProperties props = m_PhysicalDevice.getProperties();

			vk::SampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
			if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
			if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
			if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
			if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
			if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
			if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

			return vk::SampleCountFlagBits::e1;
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

		// MSAA count
		vk::SampleCountFlagBits					m_MSAASamples = vk::SampleCountFlagBits::e1;

		// Pipeline class - We only need one to start
		std::unique_ptr<Pipeline>				m_TexturedPipeline;

		// Virtually identical to above, but with different push constant ranges
		std::unique_ptr<Pipeline>				m_PBRPipeline;

		// Only draws the skybox
		std::unique_ptr<Pipeline>				m_SkyboxPipeline;

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
		Scene*									m_ActiveScene = nullptr;

		// One buffer with a copy of the view projection per frame. Because we may need to update it for a new frame whilst other is in flight
		std::vector<std::unique_ptr<BaseBuffer>>	m_ViewProjectionBuffers;

		// Same as ViewProjection but for lights
		std::vector<std::unique_ptr<BaseBuffer>>	m_PointLightBuffers;

		// Descriptor pools which are used to allocate descriptor sets
		vk::UniqueDescriptorPool					m_DescriptorPool;

		// This might seem like it should go in Pipeline.
		// HOWEVER descriptor sets are not unique to a single pipeline, as long as the layout is compatitible

		// Store this as it will be the same for any pipeline we make 99% of time
		vk::DescriptorBufferInfo							m_ViewProjectionBufferInfo;
		vk::DescriptorBufferInfo							m_PointLightBufferInfo;
		PointLights											m_Lights;
		
		// Also need to cache the writes for when we update textures
		std::vector<std::array<vk::WriteDescriptorSet,3>>	m_DescriptorWrites;

		// Store the actualy sets
		std::vector<vk::DescriptorSet>						m_DescriptorSets;

		// Store sets for skybox pass
		std::vector<vk::DescriptorSet>						m_SkyboxDescriptorSets;
		
		// Used to sample textures passed in by the user on draw commands
		vk::UniqueSampler									m_TextureSampler;

		// Limitation of wanting to stick to no extensions, I need a texture to bind to by default
		Texture*											m_DefaultBindingTexture;

		// List of textures loaded by the user
		// This cannot be a map as it links by index to a renderer specific thing
		std::vector<std::pair<std::string,Texture*>>	m_Textures;
		std::vector<vk::DescriptorImageInfo>			m_TextureInfos;
		// List of PBR materials created. Each component has 5 indexes that index into m_Textures
		std::unordered_map<std::string, PBRComponent>	m_PBRMaterials;

		// Also store a list of id's to display the textures in imgui windows
		std::vector<ImTextureID>				m_TextureGUIIDs;
		
		// Depth Buffer
		vk::UniqueImage							m_DepthImage;
		vk::UniqueDeviceMemory					m_DepthMemory;
		vk::UniqueImageView						m_DepthImageView;

		// Color MSAA buffer
		vk::UniqueImage							m_ColorImage;
		vk::UniqueDeviceMemory					m_ColorMemory;
		vk::UniqueImageView						m_ColorImageView;

		// Render target for the end of first render pass
		std::vector<vk::UniqueImage>			m_FramebufferImages;
		std::vector<vk::UniqueDeviceMemory>		m_FramebufferMemories;
		std::vector<vk::UniqueImageView>		m_FramebufferImageViews;

		// Image for copy to imgui
		std::vector<ImTextureID>				m_FramebufferGUIIDs;

		bool m_EnableGUI = true;
		
		// Store all loaded meshes in a map so they can accessed easily
		std::unordered_map<std::string, BufferManager::MeshIndexer> m_Renderables;

		#pragma region ECS CALLBACKS

		void UpdatePointlightArray();
		
		#pragma endregion

		#pragma region IMGUI ADDITIONS

		vk::DescriptorPool				m_ImGuiDescriptorPool;
		vk::RenderPass					m_ImGuiRenderPass;
		vk::CommandPool					m_ImGuiCommandPool;
		std::vector<vk::CommandBuffer>	m_ImGuiCommandBuffers;
		std::vector<vk::Framebuffer>	m_ImGuiFramebuffers;
		
		#pragma endregion
		
		#pragma endregion


	};

}
