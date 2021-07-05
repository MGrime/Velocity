#pragma once

#include "imgui.h"
#include "../../Custom Controls/Controls.hpp"
#include <nfd.h>
#include <filesystem>

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
				// THIS LOGIC MAKES A LOT OF ASSUMPTIONS ABOUT THE FOLDER LAYOUT
				// BUT ITS JUST A REQUIREMENT TO SIMPLIFY
				// TODO: MAKE SURE THE SETUP OF FOLDER IS EXPLAINED
				if (ImGui::MenuItem("Material"))
				{
					nfdchar_t* selectedFolder = nullptr;

					// Open a folder selection dialog
					const nfdresult_t result = NFD_PickFolder(nullptr, &selectedFolder);
					bool bUseable = false;
					switch(result)
					{
						case NFD_OKAY:
						{
							// Processing happens later down
							bUseable = true;
							break;
						}
						case NFD_CANCEL:
						{
							// Do nothinh
							break;
						}
						case NFD_ERROR:
						{
							VEL_CORE_ERROR("Error opening file explorer! %s", std::string(NFD_GetError()));
							break;
						}
					}
					if (bUseable)
					{
						// Now we have the folder that should contain the material
						const auto fileListIterator = std::filesystem::directory_iterator(std::string(selectedFolder));
						std::vector<std::filesystem::directory_entry> fileList;

						// Get a list of all files in the folder
						for (const auto& file : fileListIterator)
						{
							fileList.push_back(file);
						}

						if (fileList.empty())
						{
							VEL_CORE_ERROR("Selected folder is empty!");
							return;
						}

						// Check for height map
						bool bHasHeightMap = false;
						for (auto& file : fileList)
						{
							// Found a file with _height in the name. ASSUMPTION
							if (file.path().generic_string().find("_height") != std::string::npos)
							{
								bHasHeightMap = true;
								break;
							}
						}

						// Use first file to extract naming convention as albedo will be alphabetically first
						auto firstFilepath = fileList.at(0).path().generic_string();
						auto fileExtension = fileList.at(0).path().extension().generic_string();
						auto refName = fileList.at(0).path().filename().generic_string();

						// Find where albedo is
						const auto erasePathPos = firstFilepath.find("_albedo");
						const auto eraseRefPos = refName.find("_albedo");

						// Erase from that till end
						firstFilepath.erase(erasePathPos, firstFilepath.length() - erasePathPos);
						refName.erase(eraseRefPos, refName.length() - eraseRefPos);

						// firstFilepath now contains the root path

						Renderer::GetRenderer()->CreatePBRMaterial(firstFilepath, fileExtension, refName, bHasHeightMap);
					}
					
				}
				if (ImGui::MenuItem("Skybox"))
				{
					nfdchar_t* selectedFolder = nullptr;

					// Open a folder selection dialog
					const nfdresult_t result = NFD_PickFolder(nullptr, &selectedFolder);
					bool bUseable = false;
					switch (result)
					{
					case NFD_OKAY:
					{
						// Processing happens later down
						bUseable = true;
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
					if (bUseable)
					{
						// Folder should contain the skybox
						auto folderpath = std::filesystem::path(std::string(selectedFolder));
						
						// Extract extension from first file
						const auto fileListIterator = std::filesystem::directory_iterator(std::string(selectedFolder));
						std::filesystem::directory_entry firstFile = {};

						// Get a list of all files in the folder
						for (const auto& file : fileListIterator)
						{
							firstFile = file;
							break;
						}

						if (!firstFile.exists())
						{
							VEL_CORE_ERROR("Selected folder is empty!");
							return;
						}

						// Check first file for extension
						auto extension = firstFile.path().extension();

						// release current skybox if one
						if (scene->GetSkybox())
						{
							scene->RemoveSkybox();
						}

						scene->CreateSkybox(folderpath.string(), extension.string());
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