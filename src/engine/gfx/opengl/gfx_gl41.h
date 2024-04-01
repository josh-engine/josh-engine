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

struct JEShaderProgram_GL41 {
    bool testDepth;
    int location_model;
    int location_normal;
    int location_view;
    int location_2dProj;
    int location_3dProj;
    int location_cameraPos;
    int location_cameraDir;
    int location_sunDir;
    int location_sunColor;
    int location_ambience;
    unsigned int glShaderProgramID;

    JEShaderProgram_GL41(unsigned int glShader, bool testDepth) {
        this->testDepth = testDepth;
        location_model = glGetUniformLocation(glShader, "model");
        location_normal = glGetUniformLocation(glShader, "normal");
        location_view = glGetUniformLocation(glShader, "viewMatrix");
        location_2dProj = glGetUniformLocation(glShader, "_2dProj");
        location_3dProj = glGetUniformLocation(glShader, "_3dProj");
        location_cameraPos = glGetUniformLocation(glShader, "cameraPos");
        location_cameraDir = glGetUniformLocation(glShader, "cameraDir");
        location_sunDir = glGetUniformLocation(glShader, "sunDir");
        location_sunColor = glGetUniformLocation(glShader, "sunColor");
        location_ambience = glGetUniformLocation(glShader, "ambience");
        glShaderProgramID = glShader;
    }
};

void initGFX(GLFWwindow** window);
void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls);
void deinitGFX();
unsigned int loadTexture(const std::string& fileName);
unsigned int loadShader(const std::string& file_path, int target);
unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth);
unsigned int loadCubemap(std::vector<std::string> faces);
void resizeViewport();

#endif //JOSHENGINE_GFX_GL41_H
#endif //GFX_API_OPENGL41