#include "GLSLCompiler.h"

#include <memory>
#include <iostream>

namespace PBEngine
{
    // Returns GLSL shader source text after preprocessing.
    std::string GLSLCompiler::preprocess_shader(const std::string& source_name,
        shaderc_shader_kind kind,
        const std::string& source) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition("MY_DEFINE", "1");

        shaderc::PreprocessedSourceCompilationResult result =
            compiler.PreprocessGlsl(source, kind, source_name.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return "";
        }

        return { result.cbegin(), result.cend() };
    }

    // Compiles a shader to SPIR-V assembly. Returns the assembly text
    // as a string.
    std::string GLSLCompiler::compile_file_to_assembly(const std::string& source_name,
        shaderc_shader_kind kind,
        const std::string& source,
        bool optimize = false) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
            source, kind, source_name.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return "";
        }

        return { result.cbegin(), result.cend() };
    }

    // Compiles a shader to a SPIR-V binary. Returns the binary as
    // a vector of 32-bit words.
    std::vector<uint32_t> GLSLCompiler::compile_file(const std::string& source_name,
        shaderc_shader_kind kind,
        const std::string& source,
        bool optimize = false) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);
        options.SetTargetSpirv(shaderc_spirv_version_1_4);

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << module.GetErrorMessage();
            return std::vector<uint32_t>();
        }

        return { module.cbegin(), module.cend() };
    }

    shaderc_shader_kind GetShaderKind(VkShaderStageFlagBits stage)
    {
        switch (stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_vertex_shader;
            break;
        /*case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            break;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            break;*/
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return shaderc_geometry_shader;
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_fragment_shader;
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_compute_shader;
            break;
        case VK_SHADER_STAGE_ALL_GRAPHICS:
            break;
        case VK_SHADER_STAGE_ALL:
            break;
        case VK_SHADER_STAGE_RAYGEN_BIT_KHR:
            return shaderc_raygen_shader;
            break;
        case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:
            return shaderc_anyhit_shader;
            break;
        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            return shaderc_closesthit_shader;
            break;
        case VK_SHADER_STAGE_MISS_BIT_KHR:
            return shaderc_miss_shader;
            break;
        case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:
            return shaderc_intersection_shader;
            break;
        case VK_SHADER_STAGE_CALLABLE_BIT_KHR:
            return shaderc_callable_shader;
            break;
        case VK_SHADER_STAGE_TASK_BIT_EXT:
            return shaderc_task_shader;
            break;
        case VK_SHADER_STAGE_MESH_BIT_EXT:
            return shaderc_mesh_shader;
            break;
        default:
            break;
        }
    }

    VkShaderModule GLSLCompiler::load_shader(const std::string& source, VkDevice device,
        VkShaderStageFlagBits stage, bool optimize)
    {
        shaderc_shader_kind kind = GetShaderKind(stage);
        std::vector<uint32_t> spirv;
        spirv = compile_file("ShaderSource", kind, source, optimize);

        VkShaderModule           shader_module;
        VkShaderModuleCreateInfo module_create_info{};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.codeSize = spirv.size() * sizeof(uint32_t);
        module_create_info.pCode = spirv.data();

        check_vk_result(vkCreateShaderModule(device, &module_create_info, NULL, &shader_module));

        return shader_module;
    }

    VkPipelineShaderStageCreateInfo GLSLCompiler::load_shader(const std::string& source,
        VkShaderStageFlagBits stage, bool optimize)
    {
        VkPipelineShaderStageCreateInfo shader_stage = {};
        shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage.stage = stage;
        shader_stage.module = load_shader(source.c_str(), GetDevice(), stage, optimize);
        shader_stage.pName = "main";
        assert(shader_stage.module != VK_NULL_HANDLE);
        return shader_stage;
    }
}