#pragma once

#include <vulkan/vulkan.hpp>

#include "Velocity/Core/Log.hpp"

namespace Velocity
{
	class TemporaryCommandBuffer
	{
	public:
		vk::CommandBuffer& GetBuffer() { return m_CommandBuffer; }
		
		TemporaryCommandBuffer(vk::UniqueDevice& device,vk::CommandPool& pool, vk::Queue& queue)
		{
			r_Device = &device;
			r_Pool = pool;
			r_Queue = queue;
			
			// Allocate a command buffer temporarily
			vk::CommandBufferAllocateInfo allocInfo = {
				pool,
				vk::CommandBufferLevel::ePrimary,
				1
			};

			auto result = device->allocateCommandBuffers(&allocInfo, &m_CommandBuffer);
			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ERROR("A buffer copy failed to allocate! Please check log!");
				VEL_CORE_ASSERT(false, "Failed to copy buffer!");
				return;
			}

			// Start a one time buffer
			vk::CommandBufferBeginInfo beginInfo = {
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit
			};
			result = m_CommandBuffer.begin(&beginInfo);

			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ERROR("A buffer copy failed to start! Please check log!");
				VEL_CORE_ASSERT(false, "Failed to start buffer copy!");
				return;
			}
		}

		~TemporaryCommandBuffer()
		{
			m_CommandBuffer.end();

			vk::SubmitInfo submitInfo = {
				{},{},{},1,&m_CommandBuffer,{},{}
			};

			vk::Result result = r_Queue.submit(1, &submitInfo, nullptr);
			if (result != vk::Result::eSuccess)
			{
				VEL_CORE_ERROR("A buffer copy failed to submit! Please check log!");
				VEL_CORE_ASSERT(false, "Failed to submit buffer copy!");
				return;
			}
			// Wait on the queue to end
			r_Queue.waitIdle();

			// Deallocate command buffer
			r_Device->get().freeCommandBuffers(r_Pool, 1, &m_CommandBuffer);
			
		}
	private:
		vk::CommandBuffer m_CommandBuffer;

		vk::UniqueDevice* r_Device;
		vk::CommandPool r_Pool;
		vk::Queue r_Queue;
	};
}
