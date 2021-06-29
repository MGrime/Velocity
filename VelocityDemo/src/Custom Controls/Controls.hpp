#pragma once

#include "imgui_internal.h"
#include "imgui/imgui.h"

#include "Velocity/ECS/Entity.hpp"

#include <unordered_map>

namespace ImGui
{
	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Velocity::Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = GetContentRegionAvail();

			PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = 20.0f;
			Separator();
			bool open = TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			PopStyleVar();
			SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (BeginPopup("ComponentSettings"))
			{
				if (MenuItem("Remove component"))
					removeComponent = true;

				EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& vec, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = GetIO();
		auto font = io.Fonts->Fonts[0];

		PushID(label.c_str());

		Columns(2);
		SetColumnWidth(0, columnWidth);
		Text(label.c_str());
		NextColumn();

		PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = font->FontSize + GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		// Push colors for the button
		PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		// The button resets the value
		if (Button("X", buttonSize))
			vec.x = resetValue;
		PopStyleColor(3);

		SameLine();
		DragFloat("##X", &vec.x, 0.1f, 0.0f, 0.0f, "%.2f");
		PopItemWidth();
		SameLine();

		PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		if (Button("Y", buttonSize))
			vec.y = resetValue;
		PopStyleColor(3);

		SameLine();
		DragFloat("##Y", &vec.y, 0.1f, 0.0f, 0.0f, "%.2f");
		PopItemWidth();
		SameLine();

		PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		if (Button("Z", buttonSize))
			vec.z = resetValue;
		PopStyleColor(3);

		SameLine();
		DragFloat("##Z", &vec.z, 0.1f, 0.0f, 0.0f, "%.2f");
		PopItemWidth();

		PopStyleVar();

		Columns(1);

		PopID();

		
	}

	static void DrawVec2Control(const std::string& label, glm::vec2& vec, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = GetIO();
		auto font = io.Fonts->Fonts[0];

		PushID(label.c_str());

		Columns(2);
		SetColumnWidth(0, columnWidth);
		Text(label.c_str());
		NextColumn();

		PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = font->FontSize + GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
		
		// Push colors for the button
		PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		// The button resets the value
		if (Button("X", buttonSize))
			vec.x = resetValue;
		PopStyleColor(3);

		SameLine();
		DragFloat("##X", &vec.x, 0.1f, 0.0f, 0.0f, "%.2f");
		PopItemWidth();
		SameLine();

		PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		if (Button("Y", buttonSize))
			vec.y = resetValue;
		PopStyleColor(3);

		SameLine();
		DragFloat("##Y", &vec.y, 0.1f, 0.0f, 0.0f, "%.2f");
		PopItemWidth();
		SameLine();

		PopStyleVar();

		Columns(1);

		PopID();
		
	}

	static void DrawFloatControl(const std::string& label,const std::string& buttonText, float& f, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = GetIO();
		auto font = io.Fonts->Fonts[0];

		PushID(label.c_str());

		Columns(2);
		SetColumnWidth(0, columnWidth);
		Text(label.c_str());
		NextColumn();

		PushMultiItemsWidths(3, CalcItemWidth());
		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = font->FontSize + GetStyle().FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		// Push colors for the button
		PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		// The button resets the value
		if (Button(buttonText.c_str(), buttonSize))
			f = resetValue;
		PopStyleColor(3);

		SameLine();
		DragFloat("##X", &f, 0.1f, 0.0f, 0.0f, "%.2f");
		PopItemWidth();
		SameLine();

		PopStyleVar();

		Columns(1);

		PopID();
	}
	
	static auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
	{
		if (values.empty()) { return false; }
		return Combo(label, currIndex, vector_getter,
			static_cast<void*>(&values), values.size());
	}	
	
}
