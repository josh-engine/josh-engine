//
// Created by Ember Lee on 3/9/24.
//

#include "../../engineconfig.h"
#ifdef GFX_API_VK
#ifndef JOSHENGINE_GFX_VK_H
#define JOSHENGINE_GFX_VK_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../renderable.h"
#include <glm/glm.hpp>
#include "../../engine.h"

// VK_SHADER_STAGE_VERTEX_BIT
#define JE_VERTEX_SHADER 0x00000001
// VK_SHADER_STAGE_FRAGMENT_BIT
#define JE_FRAGMENT_SHADER 0x00000010

// 256MiB (1024KiB = 1MiB, 1024b = 1 KiB)
#define NEW_BLOCK_MIN_SIZE 268435456

#define MAX_FRAMES_IN_FLIGHT 2

struct JEMemoryBlock_VK {
    VkDeviceMemory memory;
    uint32_t type;
    VkDeviceSize size;
    VkDeviceSize top;
    bool mapped = false;
};

struct JEAllocation_VK {
    unsigned int memoryRefID;
    VkDeviceSize size;
    VkDeviceSize offset;
};

struct JEPushConstants_VK {
    glm::mat4 model;
    glm::mat4 normal;
};

struct JEDescriptorSet_VK {
    VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
    uint32_t idRef;
};

#ifdef DEBUG_ENABLED
struct JEUniformBufferReference_VK {
    std::array<JEAllocation_VK, MAX_FRAMES_IN_FLIGHT>* alloc = nullptr;
    std::array<void*, MAX_FRAMES_IN_FLIGHT>* map = {nullptr};
};
#endif

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings settings);
void renderFrame(const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinitGFX();
unsigned int loadTexture(const std::string& fileName);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const JEShaderProgramSettings& settings);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport();
unsigned int createVBO(std::vector<JEInterleavedVertex_VK> *interleavedVertices, std::vector<unsigned int> *indices);
/* We are exposing these to the user through engine.h instead.
unsigned int createUniformBuffer(size_t bufferSize);
void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll);
*/
#ifdef DEBUG_ENABLED
std::vector<JEMemoryBlock_VK> getMemory();
void* getTex(unsigned int i);
JEUniformBufferReference_VK getBuf(unsigned int i);
unsigned int getBufCount();
#endif

#endif //JOSHENGINE_GFX_VK_H
#endif //GFX_API_VK