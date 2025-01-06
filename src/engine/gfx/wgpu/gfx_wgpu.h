//
// Created by Ember Lee on 11/27/24.
//

#ifdef GFX_API_WEBGPU
#ifndef JOSHENGINE_GFX_WGPU_H
#define JOSHENGINE_GFX_WGPU_H

#include <GLFW/glfw3.h>
#include "../renderable.h"
#include <glm/glm.hpp>
#include "../../engine.h"

namespace JE::GFX {
#define JE_VERTEX_SHADER 0
#define JE_FRAGMENT_SHADER 1

void init(GLFWwindow **window, const char* windowName, int width, int height, const GraphicsSettings &settings);
void renderFrame(const std::vector<Renderable*>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinit();
unsigned int loadTexture(const std::string& fileName, const int& samplerFilter);
unsigned int loadBundledTexture(const char* fileFirstBytePtr, size_t fileLength, const int& samplerFilter);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype, bool use2DSamplers);
unsigned int loadCubemap(const std::vector<std::string> &faces);
void resizeViewport();
unsigned int createVBO(const std::vector<InterleavedVertex> *interleavedVertices, const std::vector<unsigned int> *indices);
void setClearColor(float r, float g, float b);
unsigned int createUniformBuffer(size_t bufferSize);
void updateUniformBuffer(unsigned int id, const void* ptr, size_t size, bool updateAll);
}
#endif //JOSHENGINE_GFX_WGPU_H
#endif