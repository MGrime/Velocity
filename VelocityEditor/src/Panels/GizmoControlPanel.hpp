#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include "../Custom Controls/Controls.hpp"

class GizmoControlPanel
{
public:
	static void Init()
	{
		m_GizmoOperation = ImGuizmo::TRANSLATE;
		m_GizmoMode = ImGuizmo::MODE::WORLD;
	}
	static void Draw()
	{
		ImGui::Begin("Gizmo Control");

		auto& renderer = Velocity::Renderer::GetRenderer();

		if (ImGui::RadioButton("T",m_GizmoOperation == ImGuizmo::TRANSLATE))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
			renderer->SetGizmoOperation(m_GizmoOperation);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("R",m_GizmoOperation == ImGuizmo::ROTATE))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
			renderer->SetGizmoOperation(m_GizmoOperation);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("S", m_GizmoOperation == ImGuizmo::SCALE))
		{
			m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
			renderer->SetGizmoOperation(m_GizmoOperation);
		}
		ImGui::Separator();
		if (ImGui::RadioButton("L",m_GizmoMode == ImGuizmo::MODE::LOCAL))
		{
			m_GizmoMode = ImGuizmo::MODE::LOCAL;
			renderer->SetGizmoMode(m_GizmoMode);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("W",m_GizmoMode == ImGuizmo::MODE::WORLD))
		{
			m_GizmoMode = ImGuizmo::MODE::WORLD;
			renderer->SetGizmoMode(m_GizmoMode);
		}

		
		ImGui::End();
	}
private:
	static ImGuizmo::OPERATION m_GizmoOperation;
	static ImGuizmo::MODE m_GizmoMode;
	
};

ImGuizmo::OPERATION GizmoControlPanel::m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
ImGuizmo::MODE GizmoControlPanel::m_GizmoMode = ImGuizmo::MODE::WORLD;