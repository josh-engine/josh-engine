//
// Created by Ember Lee on 4/25/24.
//

#ifndef JOSHENGINE_GFX_MTL_H
#define JOSHENGINE_GFX_MTL_H
#ifdef GFX_API_MTL
#include <glm/glm.hpp>
#include "../../engine.h"

#define JE_VERTEX_SHADER 0
#define JE_FRAGMENT_SHADER 1

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings settings);
void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinitGFX();
unsigned int loadTexture(const std::string& fileName);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth, bool transparencySupported, bool doubleSided);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport();

#endif
#endif //JOSHENGINE_GFX_MTL_H