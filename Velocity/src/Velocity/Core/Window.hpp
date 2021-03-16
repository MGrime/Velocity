#pragma once

#include "velpch.h"

#include "Velocity/Core/Base.hpp"
#include "Velocity/Core/Events/Event.hpp"

struct GLFWwindow;

namespace Velocity
{
	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "Velocity", const uint32_t width = 800u, const uint32_t height = 600u)
			: Title(title), Width(width), Height(height) {}
	};

	class Window
	{
	public:
		using EventCallbackFunc = std::function<void(Event&)>;

		Window(const WindowProps& props);

		virtual ~Window();

		void OnUpdate();

		uint32_t GetWidth() const { return m_Data.Props.Width; }
		uint32_t GetHeight() const { return m_Data.Props.Height; }
		GLFWwindow* GetNative() const { return m_Window; }
		const std::string& GetBaseTitle() const { return m_BaseWindowTitle; }

		void SetPaused(bool value) { m_Data.Paused = true; }
		bool IsPaused() const { return m_Data.Paused; }

		void SetEventCallback(const EventCallbackFunc& callback) { m_Data.EventCallback = callback; }
		void SetWindowIcon(const std::string& imagePath);
		void SetWindowTitle(const std::string& newTitle);

	private:
		
		GLFWwindow* m_Window;

		std::string	m_BaseWindowTitle;

		struct WindowData
		{
			WindowProps Props;
			bool VSync;
			EventCallbackFunc EventCallback;
			// Set when minimised
			bool Paused = false;
		};

		WindowData m_Data;

		
	};

}