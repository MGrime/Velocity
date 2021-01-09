#pragma once

#include <Velocity.hpp>

#include "../Custom Controls/Controls.hpp"

using namespace Velocity;

class SceneViewPanel
{
public:
	static void Draw(Scene* scene)
	{
		ImGui::Begin("Scene Hierarchy");

		// Loop all entities
		for(auto& entity : scene->GetEntities())
		{
			DrawNode(scene, entity);
		}

		// Check for if we unselect
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
		{
			m_SelectedEntity = {};
		}

		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::MenuItem("Create New Entity"))
			{
				scene->CreateEntity("Empty Entity");
			}
			ImGui::EndPopup();
		}
		
		ImGui::End();

		ImGui::Begin("Entity Properties");

		if (m_SelectedEntity)
		{
			DrawEntityProps(scene, m_SelectedEntity);

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem("Add New Component"))
				{
				}
				ImGui::EndPopup();
			}
		}
		
		ImGui::End();
	}
private:
	static Entity m_SelectedEntity;

	static void DrawNode(Scene* scene,Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0);
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<uint32_t>(entity))), flags, tag.c_str()));
		if (ImGui::IsItemClicked())
		{
			m_SelectedEntity = entity;
		}

		if (opened)
		{
			ImGui::TreePop();
		}
		

	}

	static void DrawEntityProps(Scene* scene, Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
			{
				ImGui::DrawVec3Control("Translation", component.Translation);
				ImGui::DrawVec3Control("Rotation", component.Rotation);
				ImGui::DrawVec3Control("Scale", component.Scale,1.0f);
			});
		ImGui::DrawComponent<MeshComponent>("Mesh", entity, [](MeshComponent& component)
			{
				ImGui::Text("Mesh name: %s",Renderer::GetRenderer()->GetMeshName(component).c_str());
				ImGui::Text("Vertex count: %d", component.VertexCount);
				ImGui::Text("Index count: %d", component.IndexCount);
			});

		
	}
};

Entity SceneViewPanel::m_SelectedEntity = {};