//
// Created by Ember Lee on 3/9/24.
//

#ifdef GFX_API_OPENGL33
#ifndef JOSHENGINE_GFX_GL33_H
#define JOSHENGINE_GFX_GL33_H
#include "../renderable.h"
#include <GLFW/glfw3.h>

void initGFX(GLFWwindow** window);
void renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, glm::vec3 camerapos, glm::vec3 cameradir, float fieldOfViewAngle, std::vector<Renderable> renderables, int w, int h, std::vector<void (*)()> imGuiCalls);
void deinitGFX(GLFWwindow** window);
GLuint loadTexture(std::string fileName);
GLuint loadShader(const std::string file_path, int target);
GLuint createProgram(GLuint VertexShaderID, GLuint FragmentShaderID);
GLuint loadCubemap(std::vector<std::string> faces);

#endif //JOSHENGINE_GFX_GL33_H
#endif //GFX_API_OPENGL33