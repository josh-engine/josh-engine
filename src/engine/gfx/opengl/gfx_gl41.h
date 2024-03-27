//
// Created by Ember Lee on 3/9/24.
//

#include "../../engineconfig.h"
#ifdef GFX_API_OPENGL41
#ifndef JOSHENGINE_GFX_GL41_H
#define JOSHENGINE_GFX_GL41_H

#include "../renderable.h"
#include <GLFW/glfw3.h>

#define JE_VERTEX_SHADER GL_VERTEX_SHADER
#define JE_FRAGMENT_SHADER GL_FRAGMENT_SHADER

void initGFX(GLFWwindow** window);
void renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, glm::vec3 camerapos, glm::vec3 cameradir, float fieldOfViewAngle, std::vector<Renderable> renderables, int w, int h, std::vector<void (*)()> imGuiCalls);
void deinitGFX(GLFWwindow** window);
unsigned int loadTexture(std::string fileName);
unsigned int loadShader(const std::string file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID);
unsigned int loadCubemap(std::vector<std::string> faces);

#endif //JOSHENGINE_GFX_GL41_H
#endif //GFX_API_OPENGL41