#include "velpch.h"

#include "BufferManager.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Velocity
{
	BufferManager::BufferManager(vk::PhysicalDevice& pDevice, vk::UniqueDevice& device, vk::CommandPool& pool, vk::Queue& copyQueue)
	{
		// Store refernces
		r_PhysicalDevice = pDevice;
		r_LogicalDevice = &device;
		r_Pool = pool;
		r_CopyQueue = copyQueue;

		// Create buffer
		m_VertexBuffer = std::make_unique<BaseBuffer>(
			pDevice,
			device,
			DEFAULT_BUFFER_SIZE,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		// Reserve cpu verts
		m_Vertices.reserve(DEFAULT_BUFFER_SIZE / sizeof(Vertex));

		// Create buffer
		m_IndexBuffer = std::make_unique<BaseBuffer>(
			pDevice,
			device,
			DEFAULT_BUFFER_SIZE,
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);
		
		// Reserve cpu indicies
		m_Indices.reserve(DEFAULT_BUFFER_SIZE / sizeof(uint32_t));
		
	}

	BufferManager::MeshIndexer BufferManager::AddMesh(std::vector<Vertex>& verts, std::vector<uint32_t> indices)
	{
		MeshIndexer newRenderable;
		newRenderable.VertexOffset = static_cast<int32_t>(m_Vertices.size());
		newRenderable.VertexCount = static_cast<uint32_t>(verts.size());
		newRenderable.IndexStart = static_cast<uint32_t>(m_Indices.size());
		newRenderable.IndexCount = static_cast<uint32_t>(indices.size());

		// Add the new verts and indices
		m_Vertices.insert(m_Vertices.end(),verts.begin(), verts.end());
		m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());
		// Update vertex buffer
		{
			// Calculate the size of the new area
			VkDeviceSize stagingSize = newRenderable.VertexCount * sizeof(Vertex);

			// Create a CPU staging buffer
			std::unique_ptr<BaseBuffer> stagingBuffer = std::make_unique<BaseBuffer>(
				r_PhysicalDevice,
				*r_LogicalDevice,
				stagingSize,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
				);

			// Copy to staging buffer
			// 1. Map 2. Copy 3. Unmap
			void* data;
			vk::Result result = r_LogicalDevice->get().mapMemory(stagingBuffer->Memory.get(), 0, stagingSize, vk::MemoryMapFlags{}, &data);
			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ASSERT(false, "Failed to map memory!");
				VEL_CORE_ERROR("Failed to map memory!");
				return MeshIndexer();	// Return an empty renderable. will crash anyway and be logged
			}
			memcpy(data, &m_Vertices.at(newRenderable.VertexOffset), stagingSize);
			r_LogicalDevice->get().unmapMemory(stagingBuffer->Memory.get());

			// Copy to FAST buffer
			{
				TemporaryCommandBuffer bufferWrapper = TemporaryCommandBuffer(*r_LogicalDevice, r_Pool, r_CopyQueue);
				auto& commandBuffer = bufferWrapper.GetBuffer();

				vk::BufferCopy copyRegion = {
					0,
					newRenderable.VertexOffset * sizeof(Vertex),
					stagingSize
				};

				commandBuffer.copyBuffer(stagingBuffer->Buffer.get(), m_VertexBuffer->Buffer.get(), 1, &copyRegion);
			}
		}

		// Update index buffer
		{
			// Calculate the size of the new area
			VkDeviceSize stagingSize = newRenderable.IndexCount * sizeof(uint32_t);

			// Create a CPU staging buffer
			std::unique_ptr<BaseBuffer> stagingBuffer = std::make_unique<BaseBuffer>(
				r_PhysicalDevice,
				*r_LogicalDevice,
				stagingSize,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
				);

			// Copy to staging buffer
			// 1. Map 2. Copy 3. Unmap
			void* data;
			vk::Result result = r_LogicalDevice->get().mapMemory(stagingBuffer->Memory.get(), 0, stagingSize, vk::MemoryMapFlags{}, &data);
			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ERROR("Failed to map memory!");
				VEL_CORE_ASSERT(false, "Failed to map memory!");
				return MeshIndexer();	// Return an empty renderable. will crash anyway and be logged
			}
			memcpy(data, &m_Indices.at(newRenderable.IndexStart), stagingSize);
			r_LogicalDevice->get().unmapMemory(stagingBuffer->Memory.get());

			// Copy to FAST buffer
			{
				TemporaryCommandBuffer bufferWrapper = TemporaryCommandBuffer(*r_LogicalDevice, r_Pool, r_CopyQueue);
				auto& commandBuffer = bufferWrapper.GetBuffer();

				vk::BufferCopy copyRegion = {
					0,
					newRenderable.IndexStart * sizeof(uint32_t),
					stagingSize
				};

				commandBuffer.copyBuffer(stagingBuffer->Buffer.get(), m_IndexBuffer->Buffer.get(), 1, &copyRegion);
			}
		}
		
		return newRenderable;
	
	}

	BufferManager::MeshIndexer BufferManager::AddMesh(const std::string& filepath)
	{
		// Create assimp importer
		Assimp::Importer Importer;

		// Standard import flags
		unsigned int assimpFlags =
			aiProcess_GenSmoothNormals |
			aiProcess_FixInfacingNormals |
			aiProcess_GenUVCoords |
			aiProcess_TransformUVCoords |
			aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_Triangulate |
			aiProcess_ImproveCacheLocality |
			aiProcess_SortByPType |
			aiProcess_FindInvalidData |
			aiProcess_OptimizeMeshes |
			aiProcess_FindInstances |
			aiProcess_FindDegenerates |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_Debone |
			aiProcess_RemoveComponent |
			aiProcess_CalcTangentSpace;

		// Load model
		const auto* pScene = Importer.ReadFile(filepath.c_str(), assimpFlags);

		if (pScene == nullptr)
		{
			VEL_CORE_ERROR("Failed to load model: {0}, Error: {1}", filepath, Importer.GetErrorString());
			VEL_CORE_ASSERT(false, "Failed to load model: {0}, Error: {1}", filepath, Importer.GetErrorString());
			return MeshIndexer();
		}

		std::vector<Vertex>		m_ModelVertices;
		std::vector<uint32_t>	m_ModelIndices;

		// For each mesh
		for (size_t i = 0; i < pScene->mNumMeshes; ++i)
		{
			// Get mesh
			const auto& mesh = pScene->mMeshes[i];
			for (size_t j = 0; j < mesh->mNumVertices; ++j)
			{
				// Get vertex
				const auto& rawVert = mesh->mVertices[j];
				const auto& rawNorm = mesh->mNormals[j];
				const auto& rawTangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[j] : aiVector3t<float>{};
				const auto& rawUV = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][j] : aiVector3t<float>{};
				

				Vertex vert = {
					{rawVert.x, rawVert.y, rawVert.z},
					{rawNorm.x,rawNorm.y,rawNorm.z},
					{rawTangent.x,rawTangent.y,rawTangent.z},
					{rawUV.x, rawUV.y}
				};

				m_ModelVertices.push_back(vert);
			}


			for (size_t face = 0; face < mesh->mNumFaces; ++face)
			{
				// Get face
				const auto& currFace = mesh ->mFaces[face];

				// Load indices
				for (unsigned int j = 0; j < currFace.mNumIndices; ++ j)
				{
					m_ModelIndices.push_back(currFace.mIndices[j]);
				}
			}
		}

		return AddMesh(m_ModelVertices, m_ModelIndices);	
	}

	// Binds the buffers
	void BufferManager::Bind(vk::CommandBuffer& commandBuffer)
	{
		VkDeviceSize offsets[] = { 0 };
		commandBuffer.bindVertexBuffers(0, 1, &m_VertexBuffer->Buffer.get(), offsets);
		commandBuffer.bindIndexBuffer(m_IndexBuffer->Buffer.get(), 0, vk::IndexType::eUint32);
	}
}
