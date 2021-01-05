#pragma once

#include <vulkan/vulkan.hpp>

// THIS IS WHY I FORWARD DECLARE IN RENDERER
#include <Velocity/Renderer/Renderer.hpp>	

namespace Velocity
{
	// This class manages the Window surface and the swapchain images and views
	class Swapchain
	{
	public:
		#pragma region CREATION MANAGEMENT
		
		Swapchain(vk::UniqueSurfaceKHR& surface, vk::UniqueDevice& device, const vk::SwapchainCreateInfoKHR& info);

		virtual ~Swapchain();
		
		#pragma endregion

		#pragma region GETTERS
		
		// Return swapchain width as a float
		float GetWidthF() { return static_cast<float>(m_Extent.width); }

		// Return swapchain height as a float
		float GetHeightF() { return static_cast<float>(m_Extent.height); }

		vk::Extent2D GetExtent() { return m_Extent; }
		
		#pragma endregion

		#pragma region STATIC HELPERS
		// These help renderer pick the best swapchain

		// Choose the most optimal format
		static vk::SurfaceFormatKHR ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);

		// Choose the most optimal present mode
		static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& modes);

		// Choose the most optimal swap extent
		static vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
		
		#pragma endregion 

	private:

		// Creates image views from the swapchain images
		void CreateImageViews();

		// This is the handle to interface with the surface
		vk::UniqueSurfaceKHR* r_Surface;

		// This is the handle to interface with renderer physical device
		vk::UniqueDevice* r_Device;

		// This is the vulkan swapchain handle this class manages
		vk::SwapchainKHR m_Swapchain;

		// A reference to the images created by the swapchain. Implicitly cleaned up
		std::vector<vk::Image> m_SwapchainImages;

		// Image views that map directly into images above
		std::vector<vk::ImageView> m_SwapchainImageViews;

		// Store the format and the extent
		vk::Format		m_Format;
		vk::Extent2D	m_Extent;
	};
}