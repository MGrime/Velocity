#include "velpch.h"

#include "Velocity/Renderer/Pipeline.hpp"

#include "Velocity/Core/Log.hpp"

namespace Velocity
{
	Pipeline::Pipeline(vk::UniqueDevice& device, vk::GraphicsPipelineCreateInfo& pipelineInfo, vk::PipelineLayoutCreateInfo& layoutInfo, vk::RenderPassCreateInfo& renderPassInfo, vk::DescriptorSetLayoutCreateInfo& descriptorSetLayout)
	{
		// Create the descriptor sets
		try 
		{
			m_DescriptorSetLayout = device->createDescriptorSetLayoutUnique(descriptorSetLayout);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create descriptor set layout! Error: {0}", e.what());
			VEL_CORE_ERROR("Failed to create descriptor set layout! Error: {0}", e.what());
			return;
		}
		// Update the pipeline layout info
		layoutInfo.setLayoutCount = 1u;
		layoutInfo.pSetLayouts = &m_DescriptorSetLayout.get();
		
		// Create the pipeline layout
		try
		{
			m_Layout = device->createPipelineLayoutUnique(layoutInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create pipeline layout! Error: {0}",e.what());
			VEL_CORE_ERROR("Failed to create pipeline layout! Error: {0}",e.what());
			return;	// If something breaks here the program will break, but the error will be logged which means it can be spotted
		}
		
		// Create the render pass
		try
		{
			m_RenderPass = device->createRenderPassUnique(renderPassInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create render pass! Error: {0}",e.what());
			VEL_CORE_ERROR("Failed to create render pass! Error: {0}",e.what());
			return;	// If something breaks here the program will break, but the error will be logged which means it can be spotted
		}

		// Create the pipeline
		
		// Set the render pass and layout parameters
		pipelineInfo.renderPass = m_RenderPass.get();
		pipelineInfo.subpass = 0;

		pipelineInfo.layout = m_Layout.get();

		try
		{
			auto result = device->createGraphicsPipelineUnique(nullptr, pipelineInfo);
			m_Pipeline = std::move(result.value);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create pipeline! Error: {0}", e.what());
			VEL_CORE_ERROR("Failed to create pipeline! Error: {0}", e.what());
			return;	// If something breaks here the program will break, but the error will be logged which means it can be spotted
		}
		
		
	}
	void Pipeline::Bind(vk::UniqueCommandBuffer& commandBuffer, uint32_t descriptorSetCount, uint32_t descriptorSetIndex,vk::DescriptorSet& set)
	{
		commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline.get());

		commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Layout.get(), descriptorSetIndex, descriptorSetCount,&set,0u,nullptr);
		
	}

	
}

