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
		Swapchain(vk::UniqueSurfaceKHR& surface, vk::PhysicalDevice& device);

		virtual ~Swapchain();

		// STATIC HELPER FUNCTIONS
		// These help renderer pick the best swapchain

		// Choose the most optimal format
		static vk::SurfaceFormatKHR ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);

		// Choose the most optimal present mode
		static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& modes);

		// Choose the most optimal swap extent
		static vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	private:
		// This is the handle to interface with the surface
		vk::UniqueSurfaceKHR* r_Surface;

		// This is the handle to interface with renderer physical device
		vk::PhysicalDevice* r_Device;
	};
}