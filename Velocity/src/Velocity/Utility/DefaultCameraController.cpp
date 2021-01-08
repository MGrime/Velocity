#include "velpch.h"

#include "DefaultCameraController.hpp"

#include <Velocity/Utility/Input.hpp>

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

		// Movement
		auto pos = m_Camera->GetPosition();
		auto worldMat = m_Camera->GetWorldMatrix();
		if (Input::IsKeyPressed(m_Bindings.forward))
		{
			pos.x -= moveSpeed * worldMat[2][0];
			pos.y -= moveSpeed * worldMat[2][1];
			pos.z -= moveSpeed * worldMat[2][2];
		}
		if (Input::IsKeyPressed(m_Bindings.backward))
		{
			pos.x += moveSpeed * worldMat[2][0];
			pos.y += moveSpeed * worldMat[2][1];
			pos.z += moveSpeed * worldMat[2][2];
		}
		if (Input::IsKeyPressed(m_Bindings.left))
		{
			pos.x -= moveSpeed * worldMat[0][0];
			pos.y -= moveSpeed * worldMat[0][1];
			pos.z -= moveSpeed * worldMat[0][2];
		}
		if (Input::IsKeyPressed(m_Bindings.right))
		{
			pos.x += moveSpeed * worldMat[0][0];
			pos.y += moveSpeed * worldMat[0][1];
			pos.z += moveSpeed * worldMat[0][2];
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
			rot.z -= rotationSpeed;
		}
		if (Input::IsKeyPressed(m_Bindings.rotRight))
		{
			rot.z += rotationSpeed;
		}
		m_Camera->SetRotation(rot);
	}

	void DefaultCameraController::OnEvent(Event& e)
	{
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
