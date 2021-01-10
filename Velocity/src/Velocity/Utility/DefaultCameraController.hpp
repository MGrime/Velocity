#pragma once

#include <Velocity/Utility/KeyCodes.hpp>

#include <Velocity/Utility/Camera.hpp>

#include <Velocity/Core/Events/ApplicationEvent.hpp>

#include <Velocity/Utility/Timer.hpp>

namespace Velocity
{
	struct KeyBindings
	{
		int forward = VEL_KEY_W;
		int backward = VEL_KEY_S;
		int left = VEL_KEY_A;
		int right = VEL_KEY_D;
		int up = VEL_KEY_Q;
		int down = VEL_KEY_E;
		int rotDown = VEL_KEY_DOWN;
		int rotUp = VEL_KEY_UP;
		int rotRight = VEL_KEY_RIGHT;
		int rotLeft = VEL_KEY_LEFT;
		int speedUp = VEL_KEY_LEFT_SHIFT;
	};

	class DefaultCameraController
	{
	public:
		DefaultCameraController(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& rotation = glm::vec3(0.0f, 0.0f, 0.0f),
			const float& fov = glm::pi<float>() / 3.0f, const float& aspectRatio = 16.0f / 9.0f,
			const float& nearClip = 0.1f, const float& farClip = 10000.0f);
		
		void OnUpdate(Timestep deltaTime);
		void OnEvent(Event& e);

		std::unique_ptr<Camera>& GetCamera()	{ return m_Camera; }
		const std::unique_ptr<Camera>& GetCamera() const{ return m_Camera; }

		void SetBindings(KeyBindings& newBindings) { m_Bindings = newBindings; }

		void SetMoveSpeed(float moveSpeed) { m_MoveSpeed = moveSpeed; }
		void SetRotationSpeed(float rotationSpeed) { m_RotationSpeed = rotationSpeed; }
	
	private:
		// Corrects aspect ratio
		bool OnWindowResized(WindowResizeEvent& e);
		
		// Reference to our camera
		std::unique_ptr<Camera> m_Camera;

		// Controls
		KeyBindings m_Bindings = KeyBindings();

		// User set variables
		float m_MoveSpeed = 15.0f;
		float m_RotationSpeed = 90.0f;
	};
	
}
