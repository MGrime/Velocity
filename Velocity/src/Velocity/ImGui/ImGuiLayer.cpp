#include "velpch.h"

#include "ImGuiLayer.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "imgui.h"

#include "Velocity/Core/Application.hpp"

namespace Velocity
{
	void ImGuiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::GetIO().ViewportDrawCalled = false;
		
	}
	void ImGuiLayer::End()
	{
		// Finalise render data
		ImGui::EndFrame();
		ImGui::Render();

		// Manage multiviewport
		auto& io = ImGui::GetIO();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			io.ViewportDrawCalled = true;
		}
	}

}
