#include "velpch.h"

#include "Swapchain.hpp"

#include <Velocity/Core/Log.hpp>
#include <Velocity/Core/Application.hpp>

#include <GLFW/glfw3.h>
#include <cstdint>

namespace Velocity
{
	Swapchain::Swapchain(vk::UniqueSurfaceKHR& surface, vk::UniqueDevice& device, const vk::SwapchainCreateInfoKHR& info)
	{
		r_Surface = &surface;
		r_Device = &device;

		try
		{
			m_Swapchain = device->createSwapchainKHR(info);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ERROR("An error occured in creating the swapchain: {0}", e.what());
			VEL_CORE_ASSERT(false, "Failed to create swapchain! Error {0}", e.what());
		}

		// Access the images
		m_SwapchainImages = device->getSwapchainImagesKHR(m_Swapchain);

		// Store paramters
		m_Format = info.imageFormat;
		m_Extent = info.imageExtent;

		CreateImageViews();
	}

	Swapchain::~Swapchain()
	{
		for (auto view : m_SwapchainImageViews)
		{
			r_Device->get().destroyImageView(view);
		}
		r_Device->get().destroySwapchainKHR(m_Swapchain);
	}

	void Swapchain::CreateImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
		{
			vk::ImageViewCreateInfo createInfo(
				vk::ImageViewCreateFlagBits(0u),
				m_SwapchainImages.at(i),
				vk::ImageViewType::e2D,
				m_Format,
				{
					vk::ComponentSwizzle::eIdentity,
					vk::ComponentSwizzle::eIdentity,
					vk::ComponentSwizzle::eIdentity,
					vk::ComponentSwizzle::eIdentity
				},
				{
					vk::ImageAspectFlagBits::eColor,
					0u,1u,0u,1u
				}
			);
			try
			{
				m_SwapchainImageViews.at(i) = r_Device->get().createImageView(createInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ERROR("An error occured in creating the swapchain: {0}", e.what());
				VEL_CORE_ASSERT(false, "Failed to create swapchain! Error {0}", e.what());
			}
		}
	}

	uint32_t Swapchain::AcquireImage(uint64_t timeout, vk::UniqueSemaphore& semaphore, vk::Result* result)
	{
		uint32_t index = 0u;

		*result = r_Device->get().acquireNextImageKHR(m_Swapchain, timeout, semaphore.get(),nullptr,&index);

		if (*result == vk::Result::eErrorOutOfDateKHR)
		{
			return 0u;
		}
		if (*result != vk::Result::eSuccess && *result != vk::Result::eSuboptimalKHR)
		{
			VEL_CORE_ERROR("Failed to acquire swapchain image!");
			VEL_CORE_ASSERT("Failed to acquire swapchain image!");

			return 0u;
		}
		return index;

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