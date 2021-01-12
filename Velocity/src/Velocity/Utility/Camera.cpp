#include "velpch.h"

#include "Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace Velocity
{
	void Camera::UpdateMatrices()
	{
		m_WorldMatrix = translate(glm::mat4(1.0f), m_Position)
			* rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), GLOBAL_Y)
			* rotate(glm::mat4(1.0f), glm::radians(m_Rotation.x), GLOBAL_X);

		m_ViewMatrix = affineInverse(m_WorldMatrix);

		// Projection
		float tanFOVx = std::tan(m_FOVx * 0.5f);
		float scaleX = 1.0f / tanFOVx;
		float scaleY = m_AspectRatio / tanFOVx;
		float scaleZa = m_FarClip / (m_FarClip - m_NearClip);
		float scaleZb = -m_NearClip * scaleZa;

		m_ProjectionMatrix = { scaleX,   0.0f,    0.0f,   0.0f,
								0.0f, scaleY,    0.0f,   0.0f,
								0.0f,   0.0f, scaleZa,   1.0f,
								0.0f,   0.0f, scaleZb,   0.0f };

		m_ProjectionMatrix[1][1] *= -1.0f;
	}

	//
}
