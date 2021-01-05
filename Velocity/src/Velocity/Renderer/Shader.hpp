#pragma once

#include <vulkan/vulkan.hpp>

namespace Velocity
{
	// Given the nature of vulkan this might not be needed
	// However i am using as a collection of static helpers for now for readability
	class Shader
	{
	public:
		static vk::ShaderModule CreateShaderModule(vk::UniqueDevice& device,const std::string& shaderPath);

	private:
		static std::vector<char> ReadFile(const std::string& filename);
	};
}