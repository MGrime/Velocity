#pragma once

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>

namespace Velocity
{
	class Camera
	{
	public:

		Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3& rotation = glm::vec3(0.0f, 0.0f, 0.0f),
			const float& fov = glm::pi<float>() / 3.0f, const float& aspectRatio = 16.0f / 9.0f,
			const float& nearClip = 0.1f, const float& farClip = 10000.0f)
			: m_Position(position), m_Rotation(rotation), m_FOVx(fov),
			m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
		{
			UpdateMatrices();
		}

		// Getters

		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec2& GetRotation() const { return m_Rotation; }

		const glm::mat4& GetViewMatrix() { UpdateMatrices(); return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() { UpdateMatrices(); return m_ProjectionMatrix; }
		const glm::mat4& GetWorldMatrix() { UpdateMatrices(); return m_WorldMatrix; }
		
		float GetFOV() const { return m_FOVx; }
		float GetNearClip() const { return m_NearClip; }
		float GetFarClip() const { return m_FarClip; }
		float GetAspectRatio() const { return m_AspectRatio; }

		// Setter
		void SetPosition(const glm::vec3& newPos) { m_Position = newPos; }
		void IncrementPosition(const glm::vec3& posInc) { m_Position += posInc; }

		void SetRotation(const glm::vec2& newRot) { m_Rotation = newRot; }
		void IncrementRotation(const glm::vec2& rotInc) { m_Rotation += rotInc; }

		void SetFOV(float newFov) { m_FOVx = newFov; }
		void SetNearClip(float newNearClip) { m_NearClip = newNearClip; }
		void SetFarClip(float newFarClip) { m_FarClip = newFarClip; }
		void SetAspectRatio(float newAspect) { m_AspectRatio = newAspect; }

		template<class Archive>
		void save(Archive& ar) const
		{
			ar(m_Position.x, m_Position.y, m_Position.z,
				m_Rotation.x, m_Rotation.y,
				m_FOVx, m_AspectRatio, m_NearClip, m_FarClip);
		}


		template<class Archive>
		void load(Archive& ar)
		{
			ar(m_Position.x, m_Position.y, m_Position.z,
				m_Rotation.x, m_Rotation.y,
				m_FOVx, m_AspectRatio, m_NearClip, m_FarClip);

			UpdateMatrices();
		}
	
	private:
		// Constants
		const glm::vec3 GLOBAL_X = glm::vec3(1.0f, 0.0f, 0.0f);
		const glm::vec3 GLOBAL_Y = glm::vec3(0.0f, 1.0f, 0.0f);

		void UpdateMatrices();

		// Representation in world
		glm::vec3	m_Position			= glm::vec3(0.0f,0.0f,0.0f);
		glm::vec2	m_Rotation			= glm::vec2(0.0f, 0.0f);
		glm::mat4	m_WorldMatrix		= glm::mat4(1.0f);
		glm::mat4	m_ViewMatrix		= glm::mat4(1.0f);
		glm::mat4	m_ProjectionMatrix	= glm::mat4(1.0f);

		// Aspects of camera.
		float m_FOVx			= glm::pi<float>() / 3.0f;
		float m_AspectRatio		= 16.0f / 9.0f;
		float m_NearClip		= 0.1f;
		float m_FarClip			= 10000.0f;
	};
}
