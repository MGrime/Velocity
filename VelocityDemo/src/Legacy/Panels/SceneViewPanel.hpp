#pragma once

#include <Velocity.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "../../Custom Controls/Controls.hpp"
#include "ImGuizmo/ImGuizmo.h"

using namespace Velocity;

class SceneViewPanel
{
public:
	static void Draw(Scene* scene)
	{
		ImGui::Begin("Scene Details");

		if (scene)
		{
			// Output the scene details
			ImGui::Text("Name: %s", scene->GetSceneName().c_str());
			ImGui::Text("Entity count: %d", scene->GetEntities().size());
			
			const std::string skyboxState = scene->GetSkybox() ? "Yes" : "No";
			ImGui::Text("Skybox loaded: %s", skyboxState.c_str());

			ImGui::Separator();
			ImGui::Text("Entities");
			
			// Loop all entities
			for (auto& entity : scene->GetEntities())
			{
				DrawNode(entity);
			}

			// Check for if we unselect
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
			{
				m_SelectedEntity = {};
				Renderer::GetRenderer()->SetGizmoEntity(nullptr);
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
				auto result = DrawEntityProps(scene, m_SelectedEntity);
				// Could have been removed during that function call
				if (result)
				{
					DrawEntityAddComponent();
					
				}
			}
			
		}
		
		
		ImGui::End();
	}

	static Entity& GetSelectedEntity() { return m_SelectedEntity; }

	static void ClearSelectedEntity() { m_SelectedEntity = {}; }

private:
	static Entity m_SelectedEntity;

	static void DrawNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0);
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
		bool opened = ImGui::TreeNodeEx((reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<uint32_t>(entity)))), flags, tag.c_str());

		if (ImGui::IsItemClicked())
		{
			m_SelectedEntity = entity;
			Renderer::GetRenderer()->SetGizmoEntity(&m_SelectedEntity);
		}

		if (opened)
		{
			ImGui::TreePop();
		}
	}

	static bool DrawEntityProps(Scene* scene,Entity entity)
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

			ImGui::SameLine();
			// Remove button
			if (ImGui::Button("Remove"))
			{
				// Remove all references to entity
				scene->RemoveEntity(m_SelectedEntity);
				m_SelectedEntity = {};
				Renderer::GetRenderer()->SetGizmoEntity(nullptr);
				// Exit out as handle is invalid
				return false;
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
				BufferManager::MeshIndexer mesh = Renderer::GetRenderer()->GetMeshList().at(component.MeshReference);
			
				ImGui::Text("Mesh name: %s",component.MeshReference.c_str());
				ImGui::Text("Vertex count: %d", mesh.VertexCount);
				ImGui::Text("Index count: %d", mesh.IndexCount);
			});
		ImGui::DrawComponent<TextureComponent>("Texture", entity, [](TextureComponent& component)
			{
				auto& texture = Renderer::GetRenderer()->GetTexturesList().at(component.TextureID);
			
				ImGui::Text("Texture ID: %s", std::to_string(component.TextureID).c_str());
				ImGui::Text("Texture name: %s", Renderer::GetRenderer()->GetTexturesList().at(component.TextureID).first.c_str());
				Renderer::GetRenderer()->DrawTextureToGUI(texture.first, { ImGui::GetContentRegionAvail().y,ImGui::GetContentRegionAvail().y });
			});
		ImGui::DrawComponent<PointLightComponent>("Point Light", entity, [](PointLightComponent& component)
			{
				ImGui::DrawVec3Control("Position", component.Position);
				ImGui::SliderFloat("Intensity", &component.Intensity, 100.0f, 10000.0f);
				ImGui::Separator();
				ImGui::ColorPicker3("Color", &component.Color.x);
			});
		ImGui::DrawComponent<PBRComponent>("PBR Material", entity, [](PBRComponent& component)
			{
				auto& textures = Renderer::GetRenderer()->GetTexturesList();
			
				ImGui::Text("Material name: %s", component.MaterialName.c_str());
				ImGui::Text("A / N / H / M / R");
				ImVec2 imageSize = { ImGui::GetContentRegionAvail().y / 5.0f,ImGui::GetContentRegionAvail().y / 5.0f };
				
				Renderer::GetRenderer()->DrawTextureToGUI(textures.at(component.AlbedoID()).first, imageSize);
				ImGui::SameLine();
				Renderer::GetRenderer()->DrawTextureToGUI(textures.at(component.NormalID()).first, imageSize);
				ImGui::SameLine();
				if (component.HeightID() != -1)
				{
					Renderer::GetRenderer()->DrawTextureToGUI(textures.at(component.HeightID()).first, imageSize);
					ImGui::SameLine();
				}
				Renderer::GetRenderer()->DrawTextureToGUI(textures.at(component.MetallicID()).first, imageSize);
				ImGui::SameLine();
				Renderer::GetRenderer()->DrawTextureToGUI(textures.at(component.RoughnessID()).first, imageSize);
				ImGui::SameLine();



			});
		return true;
		
	}

	static void DrawEntityAddComponent()
	{
		// Get the space we have to draw
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		// Push some padding and draw a seperator line
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
		float lineHeight = 20.0f;
		ImGui::Separator();
		ImGui::PopStyleVar();

		// Make a button in the middle
		ImGui::SetCursorPosX((contentRegionAvailable.x - lineHeight * 0.5f) / 2.0f);
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("Add Component").x);
		
		if (ImGui::Button("+", ImVec2{ lineHeight,lineHeight }))
		{
			ImGui::OpenPopup("NewComponent");
		}

		// List available components. Each need their own popup
		if (ImGui::BeginPopup("NewComponent"))
		{
			// OPEN POPUP
			if (ImGui::Button("Transform"))
			{
				if (m_SelectedEntity.HasComponent<TransformComponent>())
				{
					VEL_CLIENT_WARN("Cannot add a duplicate component!");
				}
				else if (m_SelectedEntity.HasComponent<PointLightComponent>())
				{
					VEL_CLIENT_WARN("Please remove point light component before adding a transform!");
				}
				else
				{
					m_SelectedEntity.AddComponent<TransformComponent>();
				}
			}
			if (ImGui::Button("Mesh"))
			{
				ImGui::OpenPopup("NewMeshComponent");
			}
			if (ImGui::Button("Texture(Non-PBR)"))
			{
				ImGui::OpenPopup("NewTextureComponent");
			}
			if (ImGui::Button("Material(PBR)"))
			{
				ImGui::OpenPopup("NewMaterialComponent");
			}
			if (ImGui::Button("Point Light"))
			{
				if (m_SelectedEntity.HasComponent<PointLightComponent>())
				{
					VEL_CLIENT_WARN("Cannot add a duplicate component!");
				}
				else if (m_SelectedEntity.HasComponent<TransformComponent>())
				{
					VEL_CLIENT_WARN("Please remove transform before adding a point light!");
				}
				else
				{
					m_SelectedEntity.AddComponent<PointLightComponent>(glm::vec3{ 0.0f,0.0f,0.0f }, glm::vec3{ 1.0f,1.0f,1.0f });
				}
			}

			// POPUP IMPLEMENTATIONS
			if (ImGui::BeginPopup("NewMeshComponent"))
			{
				auto& meshList = Renderer::GetRenderer()->GetMeshList();

				if (ImGui::BeginCombo("Meshes", "..."))
				{
					for (auto& mesh : meshList)
					{
						// Hide any internal meshes loaded by the engine
						if (mesh.first.find("VEL_INTERNAL") == std::string::npos)
						{
							ImGui::PushID(mesh.first.c_str());
							if (ImGui::Selectable(mesh.first.c_str()))
							{
								if (m_SelectedEntity.HasComponent<MeshComponent>())
								{
									VEL_CLIENT_WARN("Cannot add a duplicate component!");
									ImGui::PopID();// Need to pop the ID here to avoid imgui crash
									break;
								}
								m_SelectedEntity.AddComponent<MeshComponent>(MeshComponent{ mesh.first });
							}
							ImGui::PopID();
						}	
					}
					ImGui::EndCombo();
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("NewTextureComponent"))
			{
				auto& textureList = Renderer::GetRenderer()->GetTexturesList();

				if (ImGui::BeginCombo("Textures","..."))
				{
					for (auto& texture : textureList)
					{
						if (texture.first.find("VEL_INTERNAL") == std::string::npos)
						{
							ImGui::PushID(texture.first.c_str());
							if (ImGui::Selectable(texture.first.c_str()))
							{
								if (m_SelectedEntity.HasComponent<TextureComponent>())
								{
									VEL_CLIENT_WARN("Cannot add a duplicate component!");
									ImGui::PopID();// Need to pop the ID here to avoid imgui crash
									break;
								}
								m_SelectedEntity.AddComponent<TextureComponent>(Renderer::GetRenderer()->GetTextureByReference(texture.first));
							}
							ImGui::PopID();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("NewMaterialComponent"))
			{
				auto materialList = Renderer::GetRenderer()->GetMaterialsList();

				if (ImGui::BeginCombo("Materials","..."))
				{
					for (auto& material : materialList)
					{
						if (material.first.find("VEL_INTERNAL") == std::string::npos)
						{
							ImGui::PushID(material.first.c_str());
							if (ImGui::Selectable(material.first.c_str()))
							{
								if (m_SelectedEntity.HasComponent<TextureComponent>())
								{
									VEL_CLIENT_WARN("Cannot add a material to a non PBR object. Remove the texture component first!");
									ImGui::PopID();
									break;
								}
								if (m_SelectedEntity.HasComponent<PBRComponent>())
								{
									VEL_CLIENT_WARN("Cannot add a duplicate component!");
									ImGui::PopID();
									break;
								}
								m_SelectedEntity.AddComponent<PBRComponent>(Renderer::GetRenderer()->GetMaterialsList().at(material.first));
							}
							ImGui::PopID();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::EndPopup();
			}
			
			// linked to original popup
			ImGui::EndPopup();
		}
		
	}
};

Entity SceneViewPanel::m_SelectedEntity = {};