#ifndef SPIRV_HELPER_H
#define SPIRV_HELPER_H

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

// MOSTLY CREDITED TO https://lxjk.github.io/2020/03/10/Translate-GLSL-to-SPIRV-for-Vulkan-at-Runtime.html
struct SpirvHelper
{
    static void InitResources(TBuiltInResource &Resources) {
        Resources.maxLights = 32;
        Resources.maxClipPlanes = 6;
        Resources.maxTextureUnits = 32;
        Resources.maxTextureCoords = 32;
        Resources.maxVertexAttribs = 64;
        Resources.maxVertexUniformComponents = 4096;
        Resources.maxVaryingFloats = 64;
        Resources.maxVertexTextureImageUnits = 32;
        Resources.maxCombinedTextureImageUnits = 80;
        Resources.maxTextureImageUnits = 32;
        Resources.maxFragmentUniformComponents = 4096;
        Resources.maxDrawBuffers = 32;
        Resources.maxVertexUniformVectors = 128;
        Resources.maxVaryingVectors = 8;
        Resources.maxFragmentUniformVectors = 16;
        Resources.maxVertexOutputVectors = 16;
        Resources.maxFragmentInputVectors = 15;
        Resources.minProgramTexelOffset = -8;
        Resources.maxProgramTexelOffset = 7;
        Resources.maxClipDistances = 8;
        Resources.maxComputeWorkGroupCountX = 65535;
        Resources.maxComputeWorkGroupCountY = 65535;
        Resources.maxComputeWorkGroupCountZ = 65535;
        Resources.maxComputeWorkGroupSizeX = 1024;
        Resources.maxComputeWorkGroupSizeY = 1024;
        Resources.maxComputeWorkGroupSizeZ = 64;
        Resources.maxComputeUniformComponents = 1024;
        Resources.maxComputeTextureImageUnits = 16;
        Resources.maxComputeImageUniforms = 8;
        Resources.maxComputeAtomicCounters = 8;
        Resources.maxComputeAtomicCounterBuffers = 1;
        Resources.maxVaryingComponents = 60;
        Resources.maxVertexOutputComponents = 64;
        Resources.maxGeometryInputComponents = 64;
        Resources.maxGeometryOutputComponents = 128;
        Resources.maxFragmentInputComponents = 128;
        Resources.maxImageUnits = 8;
        Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
        Resources.maxCombinedShaderOutputResources = 8;
        Resources.maxImageSamples = 0;
        Resources.maxVertexImageUniforms = 0;
        Resources.maxTessControlImageUniforms = 0;
        Resources.maxTessEvaluationImageUniforms = 0;
        Resources.maxGeometryImageUniforms = 0;
        Resources.maxFragmentImageUniforms = 8;
        Resources.maxCombinedImageUniforms = 8;
        Resources.maxGeometryTextureImageUnits = 16;
        Resources.maxGeometryOutputVertices = 256;
        Resources.maxGeometryTotalOutputComponents = 1024;
        Resources.maxGeometryUniformComponents = 1024;
        Resources.maxGeometryVaryingComponents = 64;
        Resources.maxTessControlInputComponents = 128;
        Resources.maxTessControlOutputComponents = 128;
        Resources.maxTessControlTextureImageUnits = 16;
        Resources.maxTessControlUniformComponents = 1024;
        Resources.maxTessControlTotalOutputComponents = 4096;
        Resources.maxTessEvaluationInputComponents = 128;
        Resources.maxTessEvaluationOutputComponents = 128;
        Resources.maxTessEvaluationTextureImageUnits = 16;
        Resources.maxTessEvaluationUniformComponents = 1024;
        Resources.maxTessPatchComponents = 120;
        Resources.maxPatchVertices = 32;
        Resources.maxTessGenLevel = 64;
        Resources.maxViewports = 16;
        Resources.maxVertexAtomicCounters = 0;
        Resources.maxTessControlAtomicCounters = 0;
        Resources.maxTessEvaluationAtomicCounters = 0;
        Resources.maxGeometryAtomicCounters = 0;
        Resources.maxFragmentAtomicCounters = 8;
        Resources.maxCombinedAtomicCounters = 8;
        Resources.maxAtomicCounterBindings = 1;
        Resources.maxVertexAtomicCounterBuffers = 0;
        Resources.maxTessControlAtomicCounterBuffers = 0;
        Resources.maxTessEvaluationAtomicCounterBuffers = 0;
        Resources.maxGeometryAtomicCounterBuffers = 0;
        Resources.maxFragmentAtomicCounterBuffers = 1;
        Resources.maxCombinedAtomicCounterBuffers = 1;
        Resources.maxAtomicCounterBufferSize = 16384;
        Resources.maxTransformFeedbackBuffers = 4;
        Resources.maxTransformFeedbackInterleavedComponents = 64;
        Resources.maxCullDistances = 8;
        Resources.maxCombinedClipAndCullDistances = 8;
        Resources.maxSamples = 4;
        Resources.maxMeshOutputVerticesNV = 256;
        Resources.maxMeshOutputPrimitivesNV = 512;
        Resources.maxMeshWorkGroupSizeX_NV = 32;
        Resources.maxMeshWorkGroupSizeY_NV = 1;
        Resources.maxMeshWorkGroupSizeZ_NV = 1;
        Resources.maxTaskWorkGroupSizeX_NV = 32;
        Resources.maxTaskWorkGroupSizeY_NV = 1;
        Resources.maxTaskWorkGroupSizeZ_NV = 1;
        Resources.maxMeshViewCountNV = 4;
        Resources.limits.nonInductiveForLoops = 1;
        Resources.limits.whileLoops = 1;
        Resources.limits.doWhileLoops = 1;
        Resources.limits.generalUniformIndexing = 1;
        Resources.limits.generalAttributeMatrixVectorIndexing = 1;
        Resources.limits.generalVaryingIndexing = 1;
        Resources.limits.generalSamplerIndexing = 1;
        Resources.limits.generalVariableIndexing = 1;
        Resources.limits.generalConstantMatrixVectorIndexing = 1;
    }
    static EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type) {

        switch (shader_type) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                return EShLangVertex;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return EShLangFragment;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return EShLangCompute;
            default:
                return EShLangVertex;
        }
    }
    // Weird blend between these sources
    // https://lxjk.github.io/2020/03/10/Translate-GLSL-to-SPIRV-for-Vulkan-at-Runtime.html
    // https://github.com/Goutch/HellbenderEngine/blob/master/platforms/vk/ShaderCompiler.cpp#L251
    // my own code (though hardly any)
    static bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int>* spirv) {
        glslang::InitializeProcess();
        EShLanguage stage = FindLanguage(shader_type);

        glslang::TShader shader(stage);

        std::string source_str = pshader;
        const char *source_str_ptr = source_str.c_str();
        const char *const *source_ptr = &source_str_ptr;
        int length = static_cast<int>(source_str.size());

        shader.setEnvClient(glslang::EShClient::EShClientNone, glslang::EShTargetVulkan_1_0);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

        shader.setStringsWithLengths(source_ptr, &length, 1);
        shader.setSourceEntryPoint("main");
        shader.setEntryPoint("main");

        auto message = static_cast<EShMessages>(EShMessages::EShMsgVulkanRules | EShMessages::EShMsgSpvRules);

        TBuiltInResource Resources = {};
        InitResources(Resources);

        if (!shader.parse(&Resources,
                          420,
                          ECompatibilityProfile,
                          false,
                          true,
                          message)) {
            std::cerr << shader.getInfoDebugLog() << std::endl;
            std::cerr << shader.getInfoLog() << std::endl;
            return false;
        }

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(message)) {
            std::cerr << program.getInfoDebugLog() << std::endl;
            std::cerr << program.getInfoLog() << std::endl;
            return false;
        }
        glslang::SpvOptions options{};
#ifdef DEBUG_ENABLED
        options.generateDebugInfo = true;
        options.stripDebugInfo = false;
		options.disableOptimizer = true;
#else
        options.generateDebugInfo=false;
        options.stripDebugInfo=true;
        options.disableOptimizer=false;
#endif
        glslang::GlslangToSpv(*program.getIntermediate(stage), *spirv, &options);
        glslang::FinalizeProcess();
        return true;
    }
};

#endif //SPIRV_HELPER_H
