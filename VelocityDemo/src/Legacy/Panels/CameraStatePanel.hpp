#pragma once

#include <Velocity/Utility/DefaultCameraController.hpp>

#include "imgui.h"
#include "../../Custom Controls/Controls.hpp"

class CameraStatePanel
{
public:
	static void Draw(Velocity::Camera* camera)
	{
		ImGui::Begin("Camera Controls");
		if (camera)
		{
			auto pos = camera->GetPosition();
			auto rot = camera->GetRotation();
			auto fov = glm::degrees(camera->GetFOV());

			ImGui::DrawVec3Control("Position: ", pos, 0.0f);
			ImGui::DrawVec2Control("Rotation: ", rot, 0.0f);
			ImGui::DrawFloatControl("FOV: ", "X", fov, 60.0f);

			camera->SetPosition(pos);
			camera->SetRotation(rot);
			camera->SetFOV(glm::radians(fov));

		}

		ImGui::End();
	}
};
