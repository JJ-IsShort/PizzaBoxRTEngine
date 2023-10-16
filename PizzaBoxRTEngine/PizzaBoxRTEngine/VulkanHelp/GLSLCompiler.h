#pragma once

#include <string>
#include "vk_common.h"
#include <shaderc/shaderc.hpp>

namespace PBEngine
{
	static class GLSLCompiler
	{
	public:
		static std::string preprocess_shader(const std::string& source_name, shaderc_shader_kind kind, const std::string& source);
		static std::string compile_file_to_assembly(const std::string& source_name, shaderc_shader_kind kind, const std::string& source, bool optimize);
		static std::vector<uint32_t> compile_file(const std::string& source_name, shaderc_shader_kind kind, const std::string& source, bool optimize);
		static VkPipelineShaderStageCreateInfo load_shader(const std::string& source, VkShaderStageFlagBits stage, bool optimize);
		static VkShaderModule load_shader(const std::string& source, VkDevice device, VkShaderStageFlagBits stage, bool optimize);

	private:
			
	};
}