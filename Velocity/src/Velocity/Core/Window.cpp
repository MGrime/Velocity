#include "velpch.h"
#include "Window.hpp"

#include "GLFW/glfw3.h"

#include "stb_image.h"

#include <Velocity/Core/Log.hpp>
#include <Velocity/Core/Events/ApplicationEvent.hpp>
#include <Velocity/Core/Events/MouseEvent.hpp>
#include <Velocity/Core/Events/KeyEvent.hpp>

#include "Velocity/Renderer/Renderer.hpp"
#include "Velocity/Utility/KeyCodes.hpp"

namespace Velocity
{
	static bool s_GLFWInit = false;

	Window::Window(const WindowProps& props)
	{
		m_Data.Props = props;

		m_BaseWindowTitle = props.Title;

		VEL_CORE_INFO("Creating window {0} Size:({1},{2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInit)
		{
			GLenum success = glfwInit();
			VEL_CORE_ASSERT(success, "Could not initalize GLFW!");

			s_GLFWInit = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height), props.Title.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, &m_Data);

		// Set callbacks

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
		});

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Props.Width = width;
				data.Props.Height = height;

				if (width == 0 && height == 0)
				{
					data.Paused = true;
				}
				else
				{
					data.Paused = false;
				}

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
		});
		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					// This has to go here otherwise it glitches, BAD
					if (key == VEL_KEY_F8)
					{
						Renderer::GetRenderer()->ToggleGUI();
					}
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
		});
		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
		});
		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				KeyTypedEvent event(keycode);
				data.EventCallback(event);
		});

	}

	Window::~Window()
	{
		glfwDestroyWindow(m_Window);
		s_GLFWInit = false;
		glfwTerminate();
	}

	void Window::OnUpdate()
	{	
		glfwPollEvents();
	}

	float Window::GetXPos() const
	{
		int xpos, ypos;
		glfwGetWindowPos(m_Window, &xpos, &ypos);
		return static_cast<float>(xpos);
	}

	float Window::GetYPos() const
	{
		int xpos, ypos;
		glfwGetWindowPos(m_Window, &xpos, &ypos);
		return static_cast<float>(ypos);
	}

	void Window::SetWindowIcon(const std::string& imagePath)
	{
		GLFWimage newIcon;
		newIcon.pixels = stbi_load(imagePath.c_str(), &newIcon.width, &newIcon.height, 0, 4);
		glfwSetWindowIcon(m_Window, 1, &newIcon);
		stbi_image_free(newIcon.pixels);

	}
	void Window::SetWindowTitle(const std::string& newTitle)
	{
		glfwSetWindowTitle(m_Window, newTitle.c_str());
	}
}
