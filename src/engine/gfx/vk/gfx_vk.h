//
// Created by Ember Lee on 3/9/24.
//

#include "../../engineconfig.h"
#ifdef GFX_API_VK
#ifndef JOSHENGINE_GFX_VK_H
#define JOSHENGINE_GFX_VK_H

#include "../renderable.h"
#include <GLFW/glfw3.h>

// VK_SHADER_STAGE_VERTEX_BIT
#define JE_VERTEX_SHADER 0x00000001
// VK_SHADER_STAGE_FRAGMENT_BIT
#define JE_FRAGMENT_SHADER 0x00000010

void initGFX(GLFWwindow** window);
void renderFrame(glm::mat4 cameraMatrix, glm::vec3 camerapos, glm::vec3 cameradir, glm::mat4 _2dProj, glm::mat4 _3dProj, std::vector<Renderable> renderables, std::vector<void (*)()> imGuiCalls);
void deinitGFX();
unsigned int loadTexture(std::string fileName);
unsigned int loadShader(const std::string file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport(int w, int h);

#endif //JOSHENGINE_GFX_VK_H
#endif //GFX_API_VK