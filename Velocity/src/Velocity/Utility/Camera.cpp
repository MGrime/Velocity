#include "velpch.h"

#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace Velocity
{
	void Camera::UpdateMatrices()
	{
		// x * z * y -> translate
		m_WorldMatrix = translate(glm::mat4(1.0f), m_Position)
			* rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), GLOBAL_Y)
			* rotate(glm::mat4(1.0f), glm::radians(m_Rotation.z), GLOBAL_Z)
			* rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), GLOBAL_X);


		m_ViewMatrix = inverse(m_WorldMatrix) * rotate(glm::mat4(1.0f),glm::radians(90.0f),glm::vec3(1.0f,0.0f,0.0f));

		// Create projection
		m_ProjectionMatrix = glm::perspective(m_FOVx, m_AspectRatio, m_NearClip, m_FarClip);
		//m_ProjectionMatrix[1][1] *= -1.0f;
	}
}
