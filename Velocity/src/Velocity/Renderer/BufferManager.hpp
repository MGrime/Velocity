#pragma once

#include "BaseBuffer.hpp"
#include "Vertex.hpp"

#include <Velocity/ECS/Components.hpp>

namespace Velocity
{
	// Stores and keeps syncronised a vertex buffer and an index buffer
	class BufferManager
	{
	public:
		// CopyQueue is 99.9% the Graphics Queue
		BufferManager(vk::PhysicalDevice& pDevice, vk::UniqueDevice& device, vk::CommandPool& pool, vk::Queue& copyQueue);

		// TODO: Update as we change how this works
		MeshComponent AddMesh(std::vector<Vertex>& verts, std::vector<uint32_t> indices);

		MeshComponent AddMesh(const std::string& filepath);

		// Binds the buffers
		void Bind(vk::CommandBuffer& commandBuffer);
	
	private:

		// By default allocate two 128mb buffers. This will change in the future as it needs too
		const static VkDeviceSize DEFAULT_BUFFER_SIZE = static_cast<VkDeviceSize>(67108864u);

		// A completely contiguous list of all vertices of all models
		std::vector<Vertex> m_Vertices;

		// As above with indicies
		std::vector<uint32_t> m_Indices;

		// The actual GPU memory buffers
		std::unique_ptr<BaseBuffer> m_VertexBuffer;
		std::unique_ptr<BaseBuffer> m_IndexBuffer;
		

		// References to renderer
		vk::PhysicalDevice r_PhysicalDevice;
		vk::UniqueDevice* r_LogicalDevice;
		vk::CommandPool r_Pool;
		vk::Queue r_CopyQueue;
	};
};