#include "velpch.h"

#include "Swapchain.hpp"

#include <Velocity/Core/Log.hpp>
#include <Velocity/Core/Application.hpp>

#include <GLFW/glfw3.h>
#include <cstdint>

namespace Velocity
{
	Swapchain::Swapchain(vk::UniqueSurfaceKHR& surface, vk::PhysicalDevice& device)
	{
		r_Surface = &surface;
		r_Device = &device;
	}


	Swapchain::~Swapchain()
	{
		
	}

	// STATIC HELPER FUNCTIONS
	// These help renderer pick the best swapchain

	// Choose the most optimal format
	vk::SurfaceFormatKHR Swapchain::ChooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
	{
		// Our best choice is:
		// B8G8R8A8Srgb - 32 bits per pixel with 8 bits per pixel
		// SrgbNonLinear - SRGB colour space
		// Default to the first one if we cant get this format
		for (const auto& format : formats)
		{
			if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return format;
			}
		}
		return formats.at(0);
	}

	// Choose the most optimal present mode
	vk::PresentModeKHR Swapchain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& modes)
	{
		// Our best choice is:
		// eMailbox - Triple buffered
		// Default to VSync if we cant get this mode
		for (const auto& mode : modes)
		{
			if (mode == vk::PresentModeKHR::eMailbox)
			{
				return mode;
			}
		}

		return vk::PresentModeKHR::eFifo;
	}

	// Choose the most optimal swap extent
	vk::Extent2D Swapchain::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		// This is catching an edge case with high DPI displays
		
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(Application::GetWindow()->GetNative(), &width, &height);

			vk::Extent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max<uint32_t>(capabilities.minImageExtent.width, std::min<uint32_t>(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max<uint32_t>(capabilities.minImageExtent.height, std::min<uint32_t>(capabilities.maxImageExtent.height, actualExtent.height));
		
			return actualExtent;
		}


		return vk::Extent2D();
	}
}