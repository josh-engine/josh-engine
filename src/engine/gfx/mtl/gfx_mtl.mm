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
#include <stb_image.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_metal.h"
#include "../imgui/imgui_impl_glfw.h"

#include "../spirv/spirv-helper.h"
#include "../spirv/cross/spirv_msl.hpp"

// include self
#include "gfx_mtl.h"

GLFWwindow** windowPtr;
JEGraphicsSettings graphicsSettings;

MTL::Device* device;
NSWindow* metalWindow;
CAMetalLayer* layer;
CA::MetalDrawable* frame;

MTL::CommandQueue* commandQueue;
MTL::CommandBuffer* commandBuffer;

std::vector<JEProgram_MTL> pipelineVec;
std::vector<JEShader_MTL> shaderVec;
std::vector<MTL::Texture*> textureVec;

std::vector<MTL::Buffer*> vertexBufferVec;
std::vector<MTL::Buffer*> indexBufferVec;

MTL::Buffer* uniformBuffer;

MTL::RenderPassDescriptor* renderPassDescriptor;
MTL::Texture* msaaRenderTargetTexture = nullptr;
MTL::Texture* depthTexture;

unsigned int loadTexture(const std::string& fileName) {
    stbi_set_flip_vertically_on_load(true);
    unsigned int textureID;
    int width, height, channels;
    unsigned char* image = stbi_load(fileName.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (image) {
        MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm_sRGB);
        textureDescriptor->setWidth(width);
        textureDescriptor->setHeight(height);
        textureDescriptor->setUsage(MTL::TextureUsageShaderRead);

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

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Metal: Failed to open file" + filename + "!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}

inline bool ends_with(std::string const& value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

unsigned int loadShader(const std::string& file_path, int target) {
    unsigned int shaderID = shaderVec.size();
    std::string metalShaderCode;
    std::vector<unsigned int> spirv_comp;
    std::vector<char> spirVCode;

    if (ends_with(file_path, ".metal") || ends_with(file_path, ".msl")) {
        std::ifstream fileStream(file_path);
        if (!fileStream.good()) {
            throw std::runtime_error("Metal: Failed to load file \"" + file_path + "\"!");
        }
        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        metalShaderCode = buffer.str();
    } else {
        std::ifstream fileStream(file_path);
        if (!fileStream.good()) {
            throw std::runtime_error("Metal: Could not find file \"" + file_path + "\"!");
        }
        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        std::string fileContents = buffer.str();
        std::cout << "Compiling " << file_path << " to SPIR-V..." << std::endl;
        bool compileSuccess = SpirvHelper::GLSLtoSPV(target, &fileContents[0], &spirv_comp);
        if (!compileSuccess) {
            throw std::runtime_error("Metal: Could not compile \"" + file_path + "\" to SPIR-V!");
        }

        spirv_cross::CompilerMSL crossCompiler(spirv_comp);
        spirv_cross::CompilerMSL::Options opts{};
        opts.vertex_index_type = spirv_cross::CompilerMSL::Options::IndexType::UInt32;
        std::cout << "Compiling " << file_path << "'s SPIR-V bytecode to MSL..." << std::endl;
        metalShaderCode = crossCompiler.compile();
    }

    NS::Error* pError = nullptr;
    std::cout << "Loading library " << file_path << "..." << std::endl;
    MTL::Library* shaderCode = device->newLibrary(NS::String::string(metalShaderCode.c_str(), NS::UTF8StringEncoding), nullptr, &pError);

    if (pError) {
        throw std::runtime_error("Metal: Failed to load library!\n" + std::string(pError->description()->cString(NS::UTF8StringEncoding)));
    }
    if(!shaderCode){
        throw std::runtime_error("Metal: Failed to load library!");
    }

    shaderVec.push_back({shaderCode, shaderCode->newFunction(NS::String::string("main0", NS::ASCIIStringEncoding))});
    return shaderID;
}

unsigned int createProgram(unsigned int vertexID, unsigned int fragID, bool testDepth, bool transparencySupported, bool doubleSided) {
    unsigned int programID = pipelineVec.size();
    MTL::VertexDescriptor* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();
    vertexDescriptor->retain();
    vertexDescriptor->reset();

    vertexDescriptor->attributes()->object(0)->setBufferIndex(2);
    vertexDescriptor->attributes()->object(0)->setOffset(offsetof(JEInterleavedVertex_MTL, position));
    vertexDescriptor->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);

    vertexDescriptor->attributes()->object(1)->setBufferIndex(2);
    vertexDescriptor->attributes()->object(1)->setOffset(offsetof(JEInterleavedVertex_MTL, textureCoordinate));
    vertexDescriptor->attributes()->object(1)->setFormat(MTL::VertexFormatFloat2);

    vertexDescriptor->attributes()->object(2)->setBufferIndex(2);
    vertexDescriptor->attributes()->object(2)->setOffset(offsetof(JEInterleavedVertex_MTL, normal));
    vertexDescriptor->attributes()->object(2)->setFormat(MTL::VertexFormatFloat3);

    vertexDescriptor->layouts()->object(2)->setStride(sizeof(JEInterleavedVertex_MTL));
    vertexDescriptor->layouts()->object(2)->setStepRate(1);
    vertexDescriptor->layouts()->object(2)->setStepFunction(MTL::VertexStepFunctionPerVertex);

    MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    renderPipelineDescriptor->setVertexFunction(shaderVec[vertexID].main);
    renderPipelineDescriptor->setFragmentFunction(shaderVec[fragID].main);
    renderPipelineDescriptor->setVertexDescriptor(vertexDescriptor);
    renderPipelineDescriptor->setSampleCount(graphicsSettings.msaaSamples);
    renderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    auto pixelFormat = (MTL::PixelFormat)layer.pixelFormat;
    renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);
    renderPipelineDescriptor->colorAttachments()->object(0)->setBlendingEnabled(transparencySupported);
    if (transparencySupported) {
        renderPipelineDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
        renderPipelineDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
        renderPipelineDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        renderPipelineDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
        renderPipelineDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        renderPipelineDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    }

    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction((testDepth ? MTL::CompareFunctionLess : MTL::CompareFunctionAlways));
    depthStencilDescriptor->setDepthWriteEnabled(!transparencySupported);

    NS::Error* error;
    pipelineVec.push_back({vertexID,
                           fragID,
                           device->newRenderPipelineState(renderPipelineDescriptor, &error),
                           device->newDepthStencilState(depthStencilDescriptor),
                           testDepth,
                           transparencySupported,
                           doubleSided});

    if (error) {
        throw std::runtime_error("Metal: Failed to create pipeline!\n" + std::string(error->description()->cString(NS::UTF8StringEncoding)));
    } else {
        std::cout << "Created pipeline " << programID << " with vertex shader " << vertexID << " and fragment shader " << fragID << std::endl;
    }

    shaderVec[vertexID].release();
    shaderVec[fragID].release();
    depthStencilDescriptor->release();
    vertexDescriptor->release();
    renderPipelineDescriptor->release();
    return programID;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    return 0;
}

unsigned int createVBO(Renderable* r) {
    unsigned int vboID = vertexBufferVec.size();

    MTL::Buffer* stagingBuffer = device->newBuffer(
            &r->interleavedVertices[0],
            r->interleavedVertices.size()*sizeof(JEInterleavedVertex_MTL),
            MTL::ResourceStorageModeShared);

    vertexBufferVec.push_back(device->newBuffer(
            r->interleavedVertices.size()*sizeof(JEInterleavedVertex_MTL),
            MTL::ResourceStorageModePrivate)
    );

    MTL::CommandBuffer* cb = commandQueue->commandBuffer();
    MTL::BlitCommandEncoder* bce = cb->blitCommandEncoder();
    bce->copyFromBuffer(stagingBuffer, 0, vertexBufferVec[vboID], 0, r->interleavedVertices.size()*sizeof(JEInterleavedVertex_MTL));
    bce->endEncoding();
    cb->addCompletedHandler(^(MTL::CommandBuffer* buf){});
    cb->commit();
    cb->waitUntilCompleted();
    stagingBuffer->release();
    cb->release();
    bce->release();

    stagingBuffer = device->newBuffer(
            &r->indices[0],
            r->indices.size()*sizeof(unsigned int),
            MTL::ResourceStorageModeShared);

    indexBufferVec.push_back(device->newBuffer(
            r->indices.size()*sizeof(unsigned int),
            MTL::ResourceStorageModePrivate)
    );

    cb = commandQueue->commandBuffer();
    bce = cb->blitCommandEncoder();
    bce->copyFromBuffer(stagingBuffer, 0, indexBufferVec[vboID], 0, r->indices.size()*sizeof(unsigned int));
    bce->endEncoding();
    cb->addCompletedHandler(^(MTL::CommandBuffer* buf){});
    cb->commit();
    cb->waitUntilCompleted();
    stagingBuffer->release();
    cb->release();
    bce->release();

    return vboID;
}

void createFrameResources(int w, int h) {
    if (graphicsSettings.msaaSamples > 1) {
        MTL::TextureDescriptor *msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
        msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
        msaaTextureDescriptor->setPixelFormat(static_cast<MTL::PixelFormat>(layer.pixelFormat));
        msaaTextureDescriptor->setWidth(w);
        msaaTextureDescriptor->setHeight(h);
        msaaTextureDescriptor->setSampleCount(graphicsSettings.msaaSamples);
        msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
        msaaTextureDescriptor->setAllowGPUOptimizedContents(true);
        msaaTextureDescriptor->setStorageMode(MTL::StorageModePrivate);

        msaaRenderTargetTexture = device->newTexture(msaaTextureDescriptor);

        msaaTextureDescriptor->release();
    }


    MTL::TextureDescriptor* depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    depthTextureDescriptor->setTextureType((graphicsSettings.msaaSamples > 1) ? MTL::TextureType2DMultisample : MTL::TextureType2D);
    depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDescriptor->setWidth(w);
    depthTextureDescriptor->setHeight(h);
    if (graphicsSettings.msaaSamples > 1) depthTextureDescriptor->setSampleCount(graphicsSettings.msaaSamples);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setAllowGPUOptimizedContents(true);
    depthTextureDescriptor->setStorageMode(MTL::StorageModePrivate);

    depthTexture = device->newTexture(depthTextureDescriptor);

    depthTextureDescriptor->release();
}

void nextFrame() {
    frame = (__bridge CA::MetalDrawable*)[layer nextDrawable];
    if (graphicsSettings.msaaSamples > 1) renderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(frame->texture());
    else renderPassDescriptor->colorAttachments()->object(0)->setTexture(frame->texture());
}

void resizeViewport() {
    int framebufferW, framebufferH;
    glfwGetFramebufferSize(*windowPtr, &framebufferW, &framebufferH);
    layer.drawableSize = CGSizeMake(framebufferW, framebufferH);

    if (msaaRenderTargetTexture) {
        msaaRenderTargetTexture->release();
        msaaRenderTargetTexture = nullptr;
    }
    if (depthTexture) {
        depthTexture->release();
        depthTexture = nullptr;
    }

    createFrameResources(framebufferW, framebufferH);
    nextFrame();
    if (graphicsSettings.msaaSamples > 1) renderPassDescriptor->colorAttachments()->object(0)->setTexture(msaaRenderTargetTexture);
    renderPassDescriptor->depthAttachment()->setTexture(depthTexture);
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

    metalWindow = glfwGetCocoaWindow(*windowPtr);
    layer = [CAMetalLayer layer];
    // Vulkan PTSD
    // Thank god I don't have to score and pick one myself this time
    device = MTL::CreateSystemDefaultDevice();
    commandQueue = device->newCommandQueue();

    layer.device = (__bridge id<MTLDevice>)device;
    layer.displaySyncEnabled = settings.vsyncEnabled;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    metalWindow.contentView.layer = layer;
    metalWindow.contentView.wantsLayer = YES;

    uniformBuffer = device->newBuffer(sizeof(JEUniformBufferObject_MTL), MTL::ResourceStorageModeShared);

    renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

    MTL::RenderPassColorAttachmentDescriptor *colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = renderPassDescriptor->depthAttachment();

    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(graphicsSettings.clearColor[0], graphicsSettings.clearColor[1],
                                                   graphicsSettings.clearColor[2], 1.0));
    if (graphicsSettings.msaaSamples > 1) {
        colorAttachment->setTexture(msaaRenderTargetTexture);
        colorAttachment->setResolveTexture(frame->texture());
        colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);
    } else {
        colorAttachment->setTexture(frame->texture());
        colorAttachment->setStoreAction(MTL::StoreActionStore);
    }

    depthAttachment->setTexture(depthTexture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(1.0);

    resizeViewport();

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
        JEUniformBufferObject_MTL ubo = {cameraMatrix, _2dProj, _3dProj, camerapos, cameradir, sundir, suncol, ambient};
        memcpy(uniformBuffer->contents(), &ubo, sizeof(JEUniformBufferObject_MTL));

        commandBuffer = commandQueue->commandBuffer();

        ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)renderPassDescriptor);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (auto func : imGuiCalls) {
            func();
        }

        ImGui::Render();

        // FOR SOME UNGODLY REASON, PROCRASTINATING THE DRAWABLE GRAB IS GOOD. THANKS METAL BEST PRACTICES.
        nextFrame();

        // Vulkan command buffer basically
        MTL::RenderCommandEncoder* renderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);
        // set facing direction
        renderCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        // UBO
        renderCommandEncoder->setVertexBuffer(uniformBuffer, 0, 0);
        renderCommandEncoder->setFragmentBuffer(uniformBuffer, 0, 0);

        int currentPipeline = -1, currentTexture = -1, currentVertexBuffer = -1;

        for (auto const& r : renderables) {
            if (r.enabled) {
                if (r.shaderProgram != currentPipeline) {
                    renderCommandEncoder->setRenderPipelineState(pipelineVec[r.shaderProgram].pipeline);
                    renderCommandEncoder->setDepthStencilState(pipelineVec[r.shaderProgram].depthStencilState);
                    if (pipelineVec[r.shaderProgram].doubleSided) {
                        renderCommandEncoder->setCullMode(MTL::CullModeNone);
                    } else {
                        renderCommandEncoder->setCullMode(MTL::CullModeBack);
                    }
                    currentPipeline = static_cast<int>(r.shaderProgram);
                }

                if (r.texture != currentTexture) {
                    renderCommandEncoder->setFragmentTexture(textureVec[r.texture], 0);
                    currentTexture = static_cast<int>(r.texture);
                }

                if (r.vboID != currentVertexBuffer) {
                    renderCommandEncoder->setVertexBuffer(vertexBufferVec[r.vboID], 0, 2);
                    currentVertexBuffer = static_cast<int>(r.vboID);
                }

                JEPerObjectBytes_MTL pushconst = {r.objectMatrix, r.rotate};
                renderCommandEncoder->setVertexBytes(&pushconst, sizeof(JEPerObjectBytes_MTL), 1);
                renderCommandEncoder->setFragmentBytes(&pushconst, sizeof(JEPerObjectBytes_MTL), 1);

                renderCommandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, r.indicesSize,
                                                            MTL::IndexTypeUInt32, indexBufferVec[r.vboID], 0);
            }
        }

        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), (__bridge id<MTLCommandBuffer>)commandBuffer, (__bridge id<MTLRenderCommandEncoder>)renderCommandEncoder);

        renderCommandEncoder->endEncoding();

        commandBuffer->presentDrawable(frame); // vkQueuePresentKHR basically but in command buffer
        commandBuffer->commit(); // send to gpu
        commandBuffer->waitUntilCompleted(); // wait for frame to finish so we don't try writing UBO
    }
}

#endif