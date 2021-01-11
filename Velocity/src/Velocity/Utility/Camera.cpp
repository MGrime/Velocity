#include "velpch.h"

#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace Velocity
{
	void Camera::UpdateMatrices()
	{
		float cosPitch = cos(glm::radians(m_Rotation.x));
		float sinPitch = sin(glm::radians(m_Rotation.x));

		float cosYaw = cos(glm::radians(m_Rotation.y));
		float sinYaw = sin(glm::radians(m_Rotation.y));

		glm::vec3 xaxis = { cosYaw, 0, -sinYaw };
		glm::vec3 yaxis = { sinYaw * sinPitch, cosPitch, cosYaw * sinPitch };
		glm::vec3 zaxis = { sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw };

		m_ViewMatrix = {
			glm::vec4(xaxis.x,            yaxis.x,            zaxis.x,      0),
			glm::vec4(xaxis.y,            yaxis.y,            zaxis.y,      0),
			glm::vec4(xaxis.z,            yaxis.z,            zaxis.z,      0),
			glm::vec4(-dot(xaxis, m_Position), -dot(yaxis, m_Position), -dot(zaxis, m_Position), 1)
		};

		m_WorldMatrix = inverse(m_ViewMatrix);
		
		// Create projection
		m_ProjectionMatrix = glm::perspective(m_FOVx, m_AspectRatio, m_NearClip, m_FarClip);
		m_ProjectionMatrix[1][1] *= -1.0f;
	}
}
