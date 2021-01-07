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
		
	}
	void ImGuiLayer::End()
	{
		// Finalise render data
		ImGui::EndFrame();
		ImGui::Render();
	}

}
