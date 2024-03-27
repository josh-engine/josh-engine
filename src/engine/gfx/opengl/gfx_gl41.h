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
void renderFrame(glm::mat4 cameraMatrix, glm::vec3 camerapos, glm::vec3 cameradir, glm::mat4 _2dProj, glm::mat4 _3dProj, std::vector<Renderable> renderables, std::vector<void (*)()> imGuiCalls);
void deinitGFX();
unsigned int loadTexture(std::string fileName);
unsigned int loadShader(const std::string file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport(int w, int h);

#endif //JOSHENGINE_GFX_GL41_H
#endif //GFX_API_OPENGL41