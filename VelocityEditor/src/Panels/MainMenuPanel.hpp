#pragma once

#include "imgui.h"
#include "../Custom Controls/Controls.hpp"

class MainMenuPanel
{
public:
	static void Draw(Scene* scene)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::MenuItem("File"))
			{
				// TODO: FIX THIS WITH NFD
				scene->SaveScene("assets/scenes/default");

				VEL_CORE_INFO("Saved out scene!");
			}
			if (ImGui::MenuItem("Edit"))
			{
				
			}
			if (ImGui::MenuItem("Settings"))
			{
				
			}
			ImGui::EndMainMenuBar();
		}

	}
};