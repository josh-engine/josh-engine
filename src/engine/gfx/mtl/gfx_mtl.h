//
// Created by Ember Lee on 4/25/24.
//

#ifndef JOSHENGINE_GFX_MTL_H
#define JOSHENGINE_GFX_MTL_H
#ifdef GFX_API_MTL
#include <glm/glm.hpp>
#include "../../engine.h"
#include <Metal/Metal.hpp>

#define JE_VERTEX_SHADER 0
#define JE_FRAGMENT_SHADER 1

struct JEShader_MTL {
    MTL::Library* code;
    MTL::Function* main;

    void release(){
        code->release();
        main->release();
    }
};

struct JEProgram_MTL {
    unsigned int vertexID;
    unsigned int fragmentID;
    MTL::RenderPipelineState* pipeline;
    MTL::DepthStencilState* depthStencilState;
    bool testDepth;
    bool transparency;
    bool doubleSided;
};

struct JEPerObjectBytes_MTL {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 normal;
};

struct JEUniformBufferObject_MTL {
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
void* getTex(unsigned int i);
#endif

#endif
#endif //JOSHENGINE_GFX_MTL_H