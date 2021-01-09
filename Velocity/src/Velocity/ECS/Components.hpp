#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Velocity
{
	struct TagComponent
	{
		std::string Tag = "";

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag) : Tag(tag){}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f,0.0f,0.0f };
		glm::vec3 Rotation = { 0.0f,0.0f,0.0f };
		glm::vec3 Scale = { 1.0f,1.0f,1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		glm::mat4 GetTransform()
		{			
			glm::mat4 rotation = rotate(glm::mat4(1.0f), glm::radians(Rotation.y), {0.0f,1.0f,0.0f})
				* rotate(glm::mat4(1.0f), glm::radians(Rotation.z), {0.0f,0.0f,1.0f})
				* rotate(glm::mat4(1.0f), glm::radians(Rotation.x), {1.0f,0.0f,0.0f});
			
			return translate(glm::mat4(1.0f), Translation) * rotation * scale(glm::mat4(1.0f), Scale);		
		}
	};

	// Submit this to the gpu to tell it you want to render this "mesh"
	struct MeshComponent
	{
		uint32_t	VertexOffset = 0;
		uint32_t	VertexCount = 0u;
		uint32_t	IndexStart = 0u;
		uint32_t	IndexCount = 0u;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;		
	};

	struct TextureComponent
	{
		uint32_t	TextureID = 0u;

		TextureComponent() = default;
		TextureComponent(const TextureComponent&) = default;
		TextureComponent(uint32_t id) { TextureID = id; }
	};

}