#pragma once

#include "imgui.h"
#include "../Custom Controls/Controls.hpp"

class MainMenuPanel
{
public:
	static void Draw()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::MenuItem("File"))
			{
				
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