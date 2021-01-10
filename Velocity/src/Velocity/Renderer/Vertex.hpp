#pragma once

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

namespace Velocity
{
	// This is a vertex in Velocity
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 UV;

		// Returns how the vertex shader should load the data from memory from vertex to vertex
		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			return {
				0,
				sizeof(Vertex),
				vk::VertexInputRate::eVertex
			};
		}

		// Returns how to extract a vertex attribute from a chunk of data
		static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions()
		{
			return {
				vk::VertexInputAttributeDescription{
					0,
					0,
					vk::Format::eR32G32B32Sfloat,
					offsetof(Vertex,Position)
				},
				vk::VertexInputAttributeDescription{
					1,
					0,
					vk::Format::eR32G32B32Sfloat,
					offsetof(Vertex,Normal)
				},
				vk::VertexInputAttributeDescription{
					2,
					0,
					vk::Format::eR32G32Sfloat,
					offsetof(Vertex,UV)					
				}
			};
		}
		
	};
}