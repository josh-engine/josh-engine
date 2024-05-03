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

struct JEAllocation_VK {
    VkDeviceMemory* memoryRef;
    VkDeviceSize size;
    VkDeviceSize offset;
};

struct JEMemoryBlock_VK {
    VkDeviceMemory memory;
    uint32_t type;
    VkDeviceSize size;
    VkDeviceSize top;
};

struct JEPushConstants_VK {
    glm::mat4 model;
    glm::mat4 normal;
};

struct JEUniformBufferObject_VK {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 _2dProj;
    alignas(16) glm::mat4 _3dProj;
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;
    alignas(16) glm::vec3 sunDir;
    alignas(16) glm::vec3 sunPos;
    alignas(16) glm::vec3 ambience;
};

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings settings);
void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinitGFX();
unsigned int loadTexture(const std::string& fileName);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth, bool transparencySupported, bool doubleSided);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport();
unsigned int createVBO(Renderable* r);
#ifdef DEBUG_ENABLED
std::vector<JEMemoryBlock_VK> getMemory();
void* getTex(unsigned int i);
#endif

#endif //JOSHENGINE_GFX_VK_H
#endif //GFX_API_VK