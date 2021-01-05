#include "velpch.h"

#include "Shader.hpp"

#include <Velocity/Core/Base.hpp>
#include <Velocity/Core/Log.hpp>

namespace Velocity {

	vk::ShaderModule Shader::CreateShaderModule(vk::UniqueDevice& device,const std::string& shaderPath)
	{
		auto shaderCode = ReadFile(shaderPath);

		vk::ShaderModuleCreateInfo createInfo(
			vk::ShaderModuleCreateFlagBits(0u),
			shaderCode.size(),
			reinterpret_cast<const uint32_t*>(shaderCode.data())
		);

		try
		{
			return device->createShaderModule(createInfo);
		}
		catch (vk::SystemError& e)
		{
			VEL_CORE_ASSERT(false, "Failed to create shader module from: {0]", shaderPath);
			VEL_CORE_ERROR("Failed to create shader module from: {0}", shaderPath);
			return nullptr;
		}


	}

	std::vector<char> Shader::ReadFile(const std::string& filename)
	{
		// Open file reading binary from the end
		std::ifstream file;
		file.open(filename, std::ios::ate | std::ios::binary);

		// Check it was accessed correctly
		if (!file.is_open())
		{
			VEL_CORE_ASSERT(false, "Failed to open file {0]", filename);
			VEL_CORE_ERROR("Failed to open file {0}", filename);
			return;
		}

		// Create a large enough buffer
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> bufferChar(fileSize);

		// Seek back to start of file and read in one go
		file.seekg(0);
		file.read(bufferChar.data(), fileSize);

		// Close and return 
		file.close();
		return bufferChar;


	}
}