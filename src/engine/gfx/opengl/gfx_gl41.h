//
// Created by Ember Lee on 3/9/24.
//

#include "../../engineconfig.h"
#ifdef GFX_API_OPENGL41
#ifndef JOSHENGINE_GFX_GL41_H
#define JOSHENGINE_GFX_GL41_H

#include "../renderable.h"
#include <GLFW/glfw3.h>
#include "../../engine.h"

#define JE_VERTEX_SHADER GL_VERTEX_SHADER
#define JE_FRAGMENT_SHADER GL_FRAGMENT_SHADER

struct JEShaderProgram_GL41 {
    bool testDepth;
    bool transparencySupported;
    bool doubleSided;

    int location_UBO;
    int location_pushConst;

    unsigned int glShaderProgramID;

    JEShaderProgram_GL41(unsigned int glShader, bool testDepth, bool transparencySupported, bool doubleSided);
};

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings graphicsSettings);
void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinitGFX();
unsigned int loadTexture(const std::string& fileName);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth, bool transparencySupported, bool doubleSided);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport();

#endif //JOSHENGINE_GFX_GL41_H
#endif //GFX_API_OPENGL41