#pragma once

#include <Velocity/Utility/DefaultCameraController.hpp>

#include "imgui.h"

class CameraStatePanel
{
public:
	static void Draw(Velocity::DefaultCameraController& cameraController)
	{
		ImGui::Begin("Camera Controls");

		auto pos = cameraController.GetCamera()->GetPosition();
		auto fov = glm::degrees(cameraController.GetCamera()->GetFOV());

		ImGui::Text("X: %f Y: %f Z: %f", pos.x, pos.y, pos.z);

		ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f);
		cameraController.GetCamera()->SetFOV(glm::radians(fov));

		if (ImGui::Button("Reset"))
		{
			cameraController.GetCamera()->SetPosition({});
			cameraController.GetCamera()->SetRotation({});
		}

		ImGui::End();
	}
};
