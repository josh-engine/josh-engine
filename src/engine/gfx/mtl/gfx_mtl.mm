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
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/QuartzCore.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../stb/stb_image.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_metal.h"
#include "../imgui/imgui_impl_glfw.h"

// include self
#include "gfx_mtl.h"

GLFWwindow** windowPtr;

MTL::Device* device;
NSWindow* metalWindow;
CAMetalLayer* layer;


MTL::CommandQueue* commandQueue;
MTL::CommandBuffer* commandBuffer;

std::vector<JEProgram_MTL> pipelineVec;
std::vector<JEShader_MTL> shaderVec;
std::vector<MTL::Texture*> textureVec;

std::vector<MTL::Buffer*> vertexBufferVec;
std::vector<MTL::Buffer*> indexBufferVec;

MTL::Buffer* uniformBuffer;
std::vector<MTL::Buffer*> perObjectBuffers;

JEGraphicsSettings graphicsSettings;

unsigned int loadTexture(const std::string& fileName) {
    stbi_set_flip_vertically_on_load(true);
    unsigned int textureID;
    int width, height, channels;
    unsigned char* image = stbi_load(fileName.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (image) {
        MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
        textureDescriptor->setWidth(width);
        textureDescriptor->setHeight(height);

        textureID = textureVec.size();
        textureVec.push_back(device->newTexture(textureDescriptor));

        MTL::Region region = MTL::Region(0, 0, 0, width, height, 1);
        NS::UInteger bytesPerRow = 4 * width;

        textureVec[textureID]->replaceRegion(region, 0, image, bytesPerRow);

        textureDescriptor->release();
        stbi_image_free(image);
    } else {
        std::cerr << "Metal: Failed to load texture " << fileName << "!" << std::endl;
        textureID = 0;
    }
    return textureID;
}

unsigned int loadShader(const std::string& file_path, int target) {
    unsigned int shaderID = shaderVec.size();
    std::ifstream fileStream(file_path);
    if (!fileStream.good()) {
        throw std::runtime_error("Metal: Failed to load file \"" + file_path + "\"!");
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    std::string fileContents = buffer.str();

    NS::Error* pError = nullptr;
    MTL::Library* shaderCode = device->newLibrary(NS::String::string(fileContents.c_str(), NS::UTF8StringEncoding), nullptr, &pError);

    if (pError) {
        throw std::runtime_error("Metal: Failed to load library!\n" + std::string(pError->description()->cString(NS::UTF8StringEncoding)));
    }
    if(!shaderCode){
        throw std::runtime_error("Metal: Failed to load library!");
    }

    commandQueue = device->newCommandQueue();

    shaderVec.push_back({shaderCode, shaderCode->newFunction(NS::String::string("_main", NS::ASCIIStringEncoding))});
    return shaderID;
}

unsigned int createProgram(unsigned int vertexID, unsigned int fragID, bool testDepth, bool transparencySupported, bool doubleSided) {
    unsigned int programID = pipelineVec.size();
    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(shaderVec[vertexID].main);
    renderPipelineDescriptor->setFragmentFunction(shaderVec[fragID].main);

    auto pixelFormat = (MTL::PixelFormat)layer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);

    NS::Error* error;
    pipelineVec.push_back({vertexID,
                           fragID,
                           device->newRenderPipelineState(renderPipelineDescriptor, &error),
                           testDepth,
                           transparencySupported,
                           doubleSided});

    renderPipelineDescriptor->release();
    return programID;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    return 0;
}

void resizeViewport() {
    int framebufferW, framebufferH;
    glfwGetFramebufferSize(*windowPtr, &framebufferW, &framebufferH);
    layer.drawableSize = CGSizeMake(framebufferW, framebufferH);
}

unsigned int createVBO(Renderable* r) {
    unsigned int vboID = vertexBufferVec.size();
    vertexBufferVec.push_back(device->newBuffer(
            &r->interleavedVertices[0],
            r->interleavedVertices.size()*sizeof(JEInterleavedVertex_MTL),
            MTL::ResourceStorageModeShared)
    );

    indexBufferVec.push_back(device->newBuffer(
            &r->indices[0],
            r->indices.size()*sizeof(unsigned int),
            MTL::ResourceStorageModeShared
    ));

    perObjectBuffers.push_back(
            device->newBuffer(
                    sizeof(JEPerObjectBuffer_MTL),
                    MTL::ResourceStorageModeShared)
    );
    return vboID;
}

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings settings) {
    windowPtr = window;
    graphicsSettings = settings;

    if (!glfwInit()) {
        throw std::runtime_error("Metal: Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    *windowPtr = glfwCreateWindow(width, height, windowName, nullptr, nullptr);

    // Vulkan PTSD
    // Thank god I don't have to score and pick one myself this time
    device = MTL::CreateSystemDefaultDevice();
    metalWindow = glfwGetCocoaWindow(*windowPtr);
    layer = [CAMetalLayer layer];

    layer.device = (__bridge id<MTLDevice>)device;
    layer.displaySyncEnabled = settings.vsyncEnabled;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    resizeViewport();

    metalWindow.contentView.layer = layer;
    metalWindow.contentView.wantsLayer = YES;

    commandQueue = device->newCommandQueue();

    uniformBuffer = device->newBuffer(sizeof(JEUniformBufferObject_MTL), MTL::ResourceStorageModeShared);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(*windowPtr, true);
    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)device);
}

void deinitGFX() {
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    device->release();
}

void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls) {
    @autoreleasepool {
        auto* frame = (__bridge CA::MetalDrawable*)[layer nextDrawable];

        JEUniformBufferObject_MTL ubo = {cameraMatrix, _2dProj, _3dProj, camerapos, cameradir, sundir, suncol, ambient};
        memcpy(uniformBuffer->contents(), &ubo, sizeof(ubo));

        commandBuffer = commandQueue->commandBuffer();

        MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
        MTL::RenderPassColorAttachmentDescriptor* colorAttachmentDescriptor = renderPassDescriptor->colorAttachments()->object(0);
        colorAttachmentDescriptor->setTexture(frame->texture());
        colorAttachmentDescriptor->setLoadAction(MTL::LoadActionClear);
        colorAttachmentDescriptor->setClearColor(MTL::ClearColor(graphicsSettings.clearColor[0], graphicsSettings.clearColor[1], graphicsSettings.clearColor[2], 1.0));
        colorAttachmentDescriptor->setStoreAction(MTL::StoreActionStore);

        // Vulkan command buffer basically
        MTL::RenderCommandEncoder* renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);

        ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)renderPassDescriptor);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (auto func : imGuiCalls) {
            func();
        }

        ImGui::Render();

        renderCommandEncoder->setVertexBuffer(uniformBuffer, 0, 1);

        for (auto const& r : renderables) {
            renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
            renderCommandEncoder->setRenderPipelineState(pipelineVec[r.shaderProgram].pipeline);
            renderCommandEncoder->setVertexBuffer(vertexBufferVec[r.vboID], 0, 0);

            JEPerObjectBuffer_MTL pushconst = {r.objectMatrix, r.rotate};
            memcpy(perObjectBuffers[r.vboID]->contents(), &pushconst, sizeof(pushconst));
            renderCommandEncoder->setVertexBuffer(perObjectBuffers[r.vboID], 0, 2);

            renderCommandEncoder->setFragmentTexture(textureVec[r.texture], 0);
            renderCommandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, r.indicesSize, MTL::IndexTypeUInt32, indexBufferVec[r.vboID], 0);
        }

        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id<MTLCommandBuffer>)commandBuffer, (__bridge id<MTLRenderCommandEncoder>)renderCommandEncoder);

        renderCommandEncoder->endEncoding();

        commandBuffer->presentDrawable(frame); // vkQueuePresentKHR basically but in command buffer
        commandBuffer->commit(); // send to gpu
        commandBuffer->waitUntilCompleted(); // wait for frame to finish

        renderPassDescriptor->release();
    }
}

#endif