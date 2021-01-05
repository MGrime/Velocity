#pragma once

#include <vulkan/vulkan.hpp>

#include <Velocity/Core/Log.hpp>

#include <Velocity/Renderer/TemporaryCommandBuffer.hpp>

namespace Velocity
{
	// Structure for a single large buffer
	struct BaseBuffer
	{
		vk::UniqueDeviceMemory	Memory;
		vk::UniqueBuffer		Buffer;
		VkDeviceSize			Size;
		
		BaseBuffer(vk::PhysicalDevice& pDevice,vk::UniqueDevice& device,VkDeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
		{
			// TODO: Does this need queue indices?
			vk::BufferCreateInfo bufferInfo = {
				vk::BufferCreateFlags{},
				size,
				usage,
				vk::SharingMode::eExclusive
			};

			try
			{
				Buffer = device->createBufferUnique(bufferInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ASSERT(false, "Failed to create buffer! Error: {0}", e.what());
				VEL_CORE_ERROR("Failed to create buffer! Error: {0}", e.what());
			}

			vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(Buffer.get());

			vk::MemoryAllocateInfo allocInfo = {
				memoryRequirements.size,
				FindMemoryType(pDevice,memoryRequirements.memoryTypeBits,properties)
			};

			try
			{
				Memory = device->allocateMemoryUnique(allocInfo);
			}
			catch (vk::SystemError& e)
			{
				VEL_CORE_ASSERT(false, "Failed to allocate buffer memory! Error: {0}", e.what());
				VEL_CORE_ERROR("Failed to allocate buffer memory! Error: {0}", e.what());
			}

			device->bindBufferMemory(Buffer.get(), Memory.get(), 0);

			Size = size;
			
		}

		// Static helper functions
		
		static uint32_t FindMemoryType(vk::PhysicalDevice& pDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
		{
			vk::PhysicalDeviceMemoryProperties memProperties = pDevice.getMemoryProperties();

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
			{
				if (typeFilter & 1 << i && (memProperties.memoryTypes.at(i).propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			VEL_CORE_ASSERT(false, "Failed to find suitable memory for buffer!");
			VEL_CORE_ERROR("Failed to find suitable memory for buffer");
			return 0u;
		}

		static void CopyBuffer(vk::UniqueDevice& device,vk::CommandPool& pool,vk::Queue queue,std::unique_ptr<BaseBuffer>& sourceBuffer, std::unique_ptr<BaseBuffer>& destinationBuffer)
		{
			auto& commandBuffer = TemporaryCommandBuffer(device, pool, queue).GetBuffer();
				
			// Copy the whole of source to destination
			vk::BufferCopy copyRegion = {
				0,
				0,
				sourceBuffer->Size
			};

			commandBuffer.copyBuffer(sourceBuffer->Buffer.get(), destinationBuffer->Buffer.get(), 1, &copyRegion);
			
		}
		
	};
}