//
// Created by Ember Lee on 3/9/24.
//

#include "../../engineconfig.h"
#ifdef GFX_API_VK
#ifndef JOSHENGINE_GFX_VK_H
#define JOSHENGINE_GFX_VK_H

#include <GLFW/glfw3.h>
#include "../renderable.h"
#include <glm/glm.hpp>
#include "../../engine.h"

namespace JE::GFX {

// VK_SHADER_STAGE_VERTEX_BIT
#define JE_VERTEX_SHADER 0x00000001
// VK_SHADER_STAGE_FRAGMENT_BIT
#define JE_FRAGMENT_SHADER 0x00000010

// 256MiB (1024KiB = 1MiB, 1024b = 1 KiB)
#define NEW_BLOCK_MIN_SIZE 268435456

#define MAX_FRAMES_IN_FLIGHT 2

struct MemoryBlock {
    VkDeviceMemory memory;
    uint32_t type;
    VkDeviceSize size;
    VkDeviceSize top;
    bool mapped = false;
};

struct Allocation {
    unsigned int memoryRefID;
    VkDeviceSize size;
    VkDeviceSize offset;
};

struct PushConstants {
    glm::mat4 model;
    glm::mat4 freeRealEstate;
};

struct DescriptorSet {
    VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
    uint32_t idRef;
};

#ifdef DEBUG_ENABLED
struct UniformBufferReference {
    std::array<Allocation, MAX_FRAMES_IN_FLIGHT>* alloc = nullptr;
    std::array<void*, MAX_FRAMES_IN_FLIGHT>* map = {nullptr};
};
#endif

void init(GLFWwindow **window, const char* windowName, int width, int height, GraphicsSettings settings);
void renderFrame(const std::vector<Renderable*>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinit();
unsigned int loadTexture(const std::string& fileName, const int& samplerFilter);
unsigned int loadBundledTexture(char* fileFirstBytePtr, size_t fileLength, const int& samplerFilter);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport();
unsigned int createVBO(std::vector<InterleavedVertex> *interleavedVertices, std::vector<unsigned int> *indices);
void setClearColor(float r, float g, float b);
unsigned int createUniformBuffer(size_t bufferSize);
void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll);
#ifdef DEBUG_ENABLED
std::vector<MemoryBlock> getMemory();
void* getTex(unsigned int i);
UniformBufferReference getBuf(unsigned int i);
unsigned int getBufCount();
#endif
} // JE::VK
#endif //JOSHENGINE_GFX_VK_H
#endif //GFX_API_VK