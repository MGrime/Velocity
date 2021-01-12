#include "velpch.h"

#include "DefaultCameraController.hpp"

#include <Velocity/Utility/Input.hpp>



#include "imgui.h"
#include "Velocity/Core/Application.hpp"

namespace Velocity
{
	DefaultCameraController::DefaultCameraController(const glm::vec3& position, const glm::vec3& rotation,
		const float& fov, const float& aspectRatio,
		const float& nearClip, const float& farClip)
	{
		m_Camera = std::make_unique<Camera>(position, rotation, fov, aspectRatio, nearClip, farClip);
	}

	void DefaultCameraController::OnUpdate(Timestep deltaTime)
	{
		// Speed up check
		float moveSpeed = m_MoveSpeed * deltaTime;
		float rotationSpeed = m_RotationSpeed * deltaTime;
		if (Input::IsKeyPressed(m_Bindings.speedUp))
		{
			moveSpeed *= 3.0f;
		}
		auto pos = m_Camera->GetPosition();
		auto worldMat = m_Camera->GetWorldMatrix();
		
		if (Input::IsKeyPressed(m_Bindings.forward))
		{
			pos.x += moveSpeed * worldMat[2].x;
			pos.y += moveSpeed * worldMat[2].y;
			pos.z += moveSpeed * worldMat[2].z;
		}
		if (Input::IsKeyPressed(m_Bindings.backward))
		{
			pos.x -= moveSpeed * worldMat[2].x;
			pos.y -= moveSpeed * worldMat[2].y;
			pos.z -= moveSpeed * worldMat[2].z;
		}
		if (Input::IsKeyPressed(m_Bindings.left))
		{
			pos.x -= moveSpeed * worldMat[0].x;
			pos.y -= moveSpeed * worldMat[0].y;
			pos.z -= moveSpeed * worldMat[0].z;
		}
		if (Input::IsKeyPressed(m_Bindings.right))
		{
			pos.x += moveSpeed * worldMat[0].x;
			pos.y += moveSpeed * worldMat[0].y;
			pos.z += moveSpeed * worldMat[0].z;
		}
		if (Input::IsKeyPressed(m_Bindings.up))
		{
			pos.x += moveSpeed * worldMat[1].x;
			pos.y += moveSpeed * worldMat[1].y;
			pos.z += moveSpeed * worldMat[1].z;
		}
		if (Input::IsKeyPressed(m_Bindings.down))
		{
			pos.x -= moveSpeed * worldMat[1].x;
			pos.y -= moveSpeed * worldMat[1].y;
			pos.z -= moveSpeed * worldMat[1].z;
		}

		m_Camera->SetPosition(pos);

		// Rotation
		auto rot = m_Camera->GetRotation();
		if (Input::IsKeyPressed(m_Bindings.rotDown))
		{
			rot.x += rotationSpeed;
		}
		if (Input::IsKeyPressed(m_Bindings.rotUp))
		{
			rot.x -= rotationSpeed;
		}
		if (Input::IsKeyPressed(m_Bindings.rotLeft))
		{
			rot.y -= rotationSpeed;
		}
		if (Input::IsKeyPressed(m_Bindings.rotRight))
		{
			rot.y += rotationSpeed;
		}
		m_Camera->SetRotation(rot);

		// When in editor and unpaused, check imgui io viewport size and adjust aspect accordingly
		if (Application::GetWindow()->IsPaused())
		{
			return;
		}
		
		auto& io = ImGui::GetIO();
		auto size = io.ViewportImageSize;
		m_Camera->SetAspectRatio(size.x / size.y);
		
	}

	void DefaultCameraController::OnEvent(Event& e)
	{
		if (Application::GetWindow()->IsPaused())
		{
			return;
		}
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(DefaultCameraController::OnWindowResized));
	}

	// Corrects aspect ratio
	bool DefaultCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		m_Camera->SetAspectRatio(static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight()));

		return false;
	}
}
