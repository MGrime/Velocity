#pragma once

#include <vulkan/vulkan.hpp>

namespace Velocity
{
	// This class stores all the details on a single pipeline
	class Pipeline
	{
	public:
		Pipeline(vk::UniqueDevice& device, vk::GraphicsPipelineCreateInfo& pipelineInfo, vk::PipelineLayoutCreateInfo& layoutInfo, vk::RenderPassCreateInfo& renderPassInfo);

		virtual ~Pipeline() = default;

		void Bind(vk::UniqueCommandBuffer& commandBuffer);
		
		vk::UniquePipeline& GetPipeline() { return m_Pipeline; }
		vk::UniqueRenderPass& GetRenderPass() { return m_RenderPass; }
		vk::UniquePipelineLayout& GetLayout() { return m_Layout; }

	private:
		// Handle to interface with the renderer device
		vk::UniqueDevice* r_Device;

		// Store the render pass
		vk::UniqueRenderPass m_RenderPass;

		// Store the layout
		vk::UniquePipelineLayout m_Layout;

		// Store the pipeline handle
		vk::UniquePipeline m_Pipeline;
	};
}