#include "velpch.h"

#include "DefaultCameraController.hpp"

#include <Velocity/Utility/Input.hpp>



#include "imgui.h"
#include "Velocity/Core/Application.hpp"
#include "Velocity/Renderer/Renderer.hpp"

namespace Velocity
{

	void DefaultCameraController::OnUpdate(Timestep deltaTime)
	{
		// Speed up check
		float moveSpeed = m_MoveSpeed * deltaTime;
		float rotationSpeed = m_RotationSpeed * deltaTime;
		if (Input::IsKeyHeld(m_Bindings.speedUp))
		{
			moveSpeed *= 3.0f;
		}
		auto pos = m_Camera->GetPosition();
		auto worldMat = m_Camera->GetWorldMatrix();
		
		if (Input::IsKeyHeld(m_Bindings.forward))
		{
			pos.x += moveSpeed * worldMat[2].x;
			pos.y += moveSpeed * worldMat[2].y;
			pos.z += moveSpeed * worldMat[2].z;
		}
		if (Input::IsKeyHeld(m_Bindings.backward))
		{
			pos.x -= moveSpeed * worldMat[2].x;
			pos.y -= moveSpeed * worldMat[2].y;
			pos.z -= moveSpeed * worldMat[2].z;
		}
		if (Input::IsKeyHeld(m_Bindings.left))
		{
			pos.x -= moveSpeed * worldMat[0].x;
			pos.y -= moveSpeed * worldMat[0].y;
			pos.z -= moveSpeed * worldMat[0].z;
		}
		if (Input::IsKeyHeld(m_Bindings.right))
		{
			pos.x += moveSpeed * worldMat[0].x;
			pos.y += moveSpeed * worldMat[0].y;
			pos.z += moveSpeed * worldMat[0].z;
		}
		if (Input::IsKeyHeld(m_Bindings.up))
		{
			pos.x += moveSpeed * worldMat[1].x;
			pos.y += moveSpeed * worldMat[1].y;
			pos.z += moveSpeed * worldMat[1].z;
		}
		if (Input::IsKeyHeld(m_Bindings.down))
		{
			pos.x -= moveSpeed * worldMat[1].x;
			pos.y -= moveSpeed * worldMat[1].y;
			pos.z -= moveSpeed * worldMat[1].z;
		}

		m_Camera->SetPosition(pos);

		// Rotation
		auto rot = m_Camera->GetRotation();
		if (Input::IsKeyHeld(m_Bindings.rotDown))
		{
			rot.x += rotationSpeed;
		}
		if (Input::IsKeyHeld(m_Bindings.rotUp))
		{
			rot.x -= rotationSpeed;
		}
		if (Input::IsKeyHeld(m_Bindings.rotLeft))
		{
			rot.y -= rotationSpeed;
		}
		if (Input::IsKeyHeld(m_Bindings.rotRight))
		{
			rot.y += rotationSpeed;
		}
		m_Camera->SetRotation(rot);

		// When unpaused or in editor, check imgui io viewport size and adjust aspect accordingly
		if (Application::GetWindow()->IsPaused() || !Renderer::GetRenderer()->m_EnableGUI)
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
