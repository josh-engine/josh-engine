//
// Created by Ember Lee on 4/25/24.
//

#ifdef GFX_API_MTL

// GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

// Metal
#include <Metal/Metal.hpp>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/QuartzCore.hpp>

// include self
#include "gfx_mtl.h"


GLFWwindow** windowPtr;
MTL::Device* physicalDevice;
NSWindow* metalWindow;
CAMetalLayer* layer;

unsigned int loadTexture(const std::string& fileName) {
    return 0;
}

unsigned int loadShader(const std::string& file_path, int target) {
    return 0;
}

unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth, bool transparencySupported, bool doubleSided) {
    return 0;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    return 0;
}

void resizeViewport() {

}

std::string hello_triangle_shader = "#include <metal_stdlib>\n"
                                    "using namespace metal;\n"
                                    "\n"
                                    "vertex float4\n"
                                    "vertexShader(uint vertexID [[vertex_id]],\n"
                                    "             constant simd::float3* vertexPositions)\n"
                                    "{\n"
                                    "    float4 vertexOutPositions = float4(vertexPositions[vertexID][0],\n"
                                    "                                       vertexPositions[vertexID][1],\n"
                                    "                                       vertexPositions[vertexID][2],\n"
                                    "                                       1.0f);\n"
                                    "    return vertexOutPositions;\n"
                                    "}\n"
                                    "\n"
                                    "fragment float4 fragmentShader(float4 vertexOutPositions [[stage_in]]) {\n"
                                    "    return float4(182.0f/255.0f, 240.0f/255.0f, 228.0f/255.0f, 1.0f);\n"
                                    "}";

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings settings) {
    windowPtr = window;

    if (!glfwInit()) {
        throw std::runtime_error("Metal: Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    *windowPtr = glfwCreateWindow(width, height, windowName, nullptr, nullptr);

    // Vulkan PTSD
    // Thank god I don't have to score and pick one myself this time
    physicalDevice = MTL::CreateSystemDefaultDevice();

    metalWindow = glfwGetCocoaWindow(*windowPtr);

    layer = [CAMetalLayer layer];
    layer.device = (__bridge id<MTLDevice>)physicalDevice;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalWindow.contentView.layer = layer;
    metalWindow.contentView.wantsLayer = YES;
}

void deinitGFX() {
    glfwTerminate();
    physicalDevice->release();
}

void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls) {

}

#endif