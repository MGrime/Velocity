#pragma once

#include "imgui.h"
#include "../Custom Controls/Controls.hpp"
#include <nfd.h>

class MainMenuPanel
{
public:
	static void Draw(Scene* scene,bool* newSceneLoaded, std::string* newScenePath, bool* sceneSaved, bool* sceneClosed)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene"))
				{
					*newSceneLoaded = true;
					*newScenePath = "";
				}
				
				if (ImGui::MenuItem("Open Scene"))
				{
					nfdchar_t* outFile = OpenFile("velocity");

					if (outFile)
					{
						*newSceneLoaded = true;
						*newScenePath = std::string(outFile);
					}
				}
				// Save only enabled if scene isnt null
				if (ImGui::MenuItem("Save Scene",0,(bool*)0,scene != nullptr))
				{
					nfdchar_t* outPath = nullptr;
					// Open a save file dialog
					nfdresult_t result = NFD_SaveDialog("velocity", nullptr, &outPath);

					switch(result)
					{
						case NFD_OKAY:
						{
							// Save to the file they said
							std::string saveLoc = std::string(outPath);
							if (saveLoc.find(".velocity") == std::string::npos)
							{
								saveLoc += ".velocity";
							}
							scene->SaveScene(std::string(saveLoc));
							*sceneSaved = true;
							VEL_CORE_INFO("Saved out scene!");
							break;
						}
						case NFD_CANCEL:
						{
							// Do nothing
							break;
						}
						case NFD_ERROR:
						{
							VEL_CORE_ERROR("Error opening file explorer! %s", std::string(NFD_GetError()));
							break;
						}
					}
					
				}

				if (ImGui::MenuItem("Close Scene",0,(bool*)0,scene !=nullptr))
				{
					*sceneClosed = true;
				}

				ImGui::EndMenu();
	
			}
			if (ImGui::MenuItem("Edit"))
			{
				
			}
			if (ImGui::MenuItem("Settings"))
			{
				
			}
			if (ImGui::BeginMenu("Import Asset", scene != nullptr))
			{
				if (ImGui::MenuItem("Mesh"))
				{
					nfdchar_t* outFile = OpenFile("fbx,x,obj,3ds");
					if (outFile)
					{
						// Construct to string for easier processing
						const std::string fullPath = std::string(outFile);

						const std::string refName = Scene::GetRefName(fullPath);
						
						// Now create the mesh
						Renderer::GetRenderer()->LoadMesh(fullPath, refName);
					}
				}
				if (ImGui::MenuItem("Texture"))
				{
					nfdchar_t* outFile = OpenFile("jpg,jpeg,png");
					if (outFile)
					{
						const std::string fullPath = std::string(outFile);

						const std::string refName = Scene::GetRefName(fullPath);

						// Now create the texture
						Renderer::GetRenderer()->CreateTexture(fullPath, refName);
					}
					
				}
				ImGui::EndMenu();
			}
			
			ImGui::EndMainMenuBar();
		}

	}

	static nfdchar_t* OpenFile(const nfdchar_t* filter)
	{
		nfdchar_t* outPath = nullptr;
		// Open a load file dialog
		nfdresult_t result = NFD_OpenDialog(filter, nullptr, &outPath);
		switch (result)
		{
		case NFD_OKAY:
		{
			// Open scene and return?
			return outPath;
		}
		case NFD_CANCEL:
		{
			// Do nothing
			return nullptr;
		}
		case NFD_ERROR:
		{
			VEL_CORE_ERROR("Error opening file explorer! %s", std::string(NFD_GetError()));
			return nullptr;
		}
		}
		return outPath;
	}

};