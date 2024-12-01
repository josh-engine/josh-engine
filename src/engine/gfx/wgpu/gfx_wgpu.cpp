//
// Created by Ethan Lee on 11/27/24.
//

#include "gfx_wgpu.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>

#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include "../../ui/imgui/imgui.h"
#include "../../ui/imgui/imgui_impl_glfw.h"
#include "../../ui/imgui/imgui_impl_wgpu.h"

namespace JE::GFX {

    struct PushConstants {
        glm::mat4 model;
        glm::mat4 freeRealEstate;
    };

    unsigned int createLifetimeBuffer(void* src, size_t size, WGPUBufferUsage usage, std::vector<WGPUBuffer>& vec, const char* label = "JElifetimebuffer");
    void resizePushConstantBuffer(size_t size);

    GraphicsSettings settings;
    GLFWwindow** window;

    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;

    WGPUQueue queue;

    WGPUSurface surface;
    WGPUTextureFormat surfaceFormat;

    // This follows the same design philosophy to Vulkan,
    // although I chose to stick with numerical IDs since
    // you can feasibly translate that to any API.
    std::vector<WGPUShaderModule> shaderModuleVector{};
    std::vector<WGPURenderPipeline> pipelineVector{};

    std::vector<WGPUBuffer> buffers{};

    std::vector<WGPUTexture> textures{};
    std::vector<WGPUTextureView> textureViews{};
    // Do I *really* need to create a sampler per texture? Fuck no. Does that make life easier for me? Yeah.
    // I don't want to have to add another value to the indexPair for pixel art/normal filtering
    std::vector<WGPUSampler> samplers{};

    WGPUBindGroupLayout uniformBindGroupLayout;
    WGPUBindGroupLayout textureBindGroupLayout;
    WGPUBindGroupLayout cubemapBindGroupLayout;

    std::vector<WGPUBindGroup> bindGroups{};

    struct index_pair {unsigned int resource; unsigned int bindGroup; bool isBuffer;};
    std::vector<index_pair> indexPairs{};

    // TODO: findSupportedFormats like in gfx_vk.cpp
    WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus;
    WGPUTexture depthTexture;
    WGPUTextureView depthTextureView;

    unsigned int pushConstantBufferID;
    size_t pushConstantBufferSize = sizeof(glm::mat4) * 2;

    void initGLFW(int width, int height, const char* windowName) {
        if (!glfwInit()) {
            throw std::runtime_error("WGPU: Could not initialize GLFW!");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        *window = glfwCreateWindow(width, height, windowName, nullptr, nullptr);
    }

    void createInstance() {
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
        instance = wgpuCreateInstance(nullptr);
#else
        // We're doing this Vulkan style
        WGPUInstanceDescriptor desc{};
        desc.nextInChain = nullptr;

        instance = wgpuCreateInstance(&desc);
#endif

        if (!instance) {
            throw std::runtime_error("WGPU: Could not initialize!");
        }
    }

    void createAdapter() {
        WGPURequestAdapterOptions adapterOpts{};
        adapterOpts.nextInChain = nullptr;
        adapterOpts.compatibleSurface = surface;

        bool areWeThereYet = false;

        wgpuInstanceRequestAdapter(instance, &adapterOpts, [](WGPURequestAdapterStatus status, WGPUAdapter a, char const * message, void * b) {
            if (status == WGPURequestAdapterStatus_Success) {
                adapter = a;
            } else {
                std::cerr << "Could not get WebGPU adapter: " << message << std::endl;
            }
            *reinterpret_cast<bool*>(b) = true;
        }, &areWeThereYet);

#ifdef __EMSCRIPTEN__
        // Horrible, bad, horrible things.
        while (!areWeThereYet) {
            emscripten_sleep(100);
        }
#endif // __EMSCRIPTEN__

        assert(areWeThereYet); // Well, we should be...

        if (adapter == nullptr) {
            throw std::runtime_error("WGPU: Failed to get adapter!");
        }
    }

    /* Boilerplate? I hardly know her!
    void setDefaultLimits(WGPULimits &limits) {
        limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
        limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
        limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
        limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
        limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
    }

    WGPURequiredLimits getRequiredLimits() {
        WGPUSupportedLimits supportedLimits{};
        supportedLimits.nextInChain = nullptr;
        wgpuAdapterGetLimits(adapter, &supportedLimits);

        WGPURequiredLimits requiredLimits{};
        setDefaultLimits(requiredLimits.limits);

        // We use at most 1 vertex attribute for now
        requiredLimits.limits.maxVertexAttributes = 1;
        // We should also tell that we use 1 vertex buffers
        requiredLimits.limits.maxVertexBuffers = 1;
        // Maximum size of a buffer is 6 vertices of 2 float each
        requiredLimits.limits.maxBufferSize = 6 * 2 * sizeof(float);
        // Maximum stride between 2 consecutive vertices in the vertex buffer
        requiredLimits.limits.maxVertexBufferArrayStride = 2 * sizeof(float);

        requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
        requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;

        return requiredLimits;
    }*/

    void createDevice() {

        WGPUDeviceDescriptor deviceDesc;
        deviceDesc.label = "John Graphics"; // why do these things have names??
        deviceDesc.requiredFeatureCount = 0;
        deviceDesc.requiredFeatures = nullptr;
        deviceDesc.requiredLimits = nullptr; // Weirdly enough, the tutorial tells me to do this then doesn't...
        deviceDesc.defaultQueue.nextInChain = nullptr;
        deviceDesc.defaultQueue.label = "John Queue"; // what??
        deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void*) {
            std::cout << "WGPU: Device lost,  " << reason;
            if (message) std::cout << " (" << message << ")";
            std::cout << std::endl;
        };;

        bool awty = false;

        wgpuAdapterRequestDevice(adapter, &deviceDesc, [](WGPURequestDeviceStatus status, WGPUDevice d, char const * message, void * b) {
            if (status == WGPURequestDeviceStatus_Success) {
                device = d;
            } else {
                std::cerr << "Could not get WebGPU device: " << message << std::endl;
            }
            *reinterpret_cast<bool*>(b) = true;
        }, &awty);

#ifdef __EMSCRIPTEN__
        while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif // __EMSCRIPTEN__

        assert(awty);

        if (device == nullptr) {
            throw std::runtime_error("WGPU: Failed to get device!");
        }

        wgpuDeviceSetUncapturedErrorCallback(device, [](WGPUErrorType type, char const* message, void*) {
            std::cout << "Uncaptured device error: type " << type;
            if (message) std::cout << " (" << message << ")";
            std::cout << std::endl;
        }, nullptr);
    }

    void createCommandObjects() {
        // Weirdly enough, it really is that simple this time. Feels like a crime.
        queue = wgpuDeviceGetQueue(device);
    }

    void createSurface() {
        surface = glfwGetWGPUSurface(instance, *window);

        if (surface == nullptr) {
            throw std::runtime_error("WGPU: Failed to get surface!");
        }
    }

    WGPUPresentMode getBestPresentMode() {
        if (settings.vsyncEnabled) {
            WGPUSurfaceCapabilities surfaceCapabilities;
            wgpuSurfaceGetCapabilities(surface, adapter, &surfaceCapabilities);

            for (int i = 0; i < surfaceCapabilities.presentModeCount; ++i) {
                if (surfaceCapabilities.presentModes[i] == WGPUPresentMode_Mailbox) {
                    return WGPUPresentMode_Mailbox;
                }
            }
            return WGPUPresentMode_Fifo;
        } else {
            // Who gives a crap about tearing
            return WGPUPresentMode_Immediate;
        }
    }

    void setDefaultBinding(WGPUBindGroupLayoutEntry &bindingLayout) {
        bindingLayout.buffer.nextInChain = nullptr;
        bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
        bindingLayout.buffer.hasDynamicOffset = false;

        bindingLayout.sampler.nextInChain = nullptr;
        bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

        bindingLayout.storageTexture.nextInChain = nullptr;
        bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
        bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
        bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

        bindingLayout.texture.nextInChain = nullptr;
        bindingLayout.texture.multisampled = false;
        bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
        bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
    }

    void createUniformBindGroupLayout() {
        // Define binding layout
        WGPUBindGroupLayoutEntry bindingLayout{};
        setDefaultBinding(bindingLayout);
        bindingLayout.binding = 0;
        bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
        bindingLayout.buffer.hasDynamicOffset = true;

        // Create a bind group layout
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.nextInChain = nullptr;
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = &bindingLayout;
        uniformBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
    }

    void createTextureBindGroupLayout() {
        std::array<WGPUBindGroupLayoutEntry, 2> layouts{};
        // Define binding layout
        setDefaultBinding(layouts[0]);
        layouts[0].binding = 0;
        layouts[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        layouts[0].texture.sampleType = WGPUTextureSampleType_Float;
        layouts[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        setDefaultBinding(layouts[1]);
        layouts[1].binding = 1;
        layouts[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        layouts[1].sampler.type = WGPUSamplerBindingType_Filtering;

        // Create a bind group layout
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.nextInChain = nullptr;
        bindGroupLayoutDesc.entryCount = layouts.size();
        bindGroupLayoutDesc.entries = layouts.data();
        textureBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
    }

    void createCubemapBindGroupLayout() {
        std::array<WGPUBindGroupLayoutEntry, 2> layouts{};
        // Define binding layout
        setDefaultBinding(layouts[0]);
        layouts[0].binding = 0;
        layouts[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        layouts[0].texture.sampleType = WGPUTextureSampleType_Float;
        layouts[0].texture.viewDimension = WGPUTextureViewDimension_Cube;

        setDefaultBinding(layouts[1]);
        layouts[1].binding = 1;
        layouts[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        layouts[1].sampler.type = WGPUSamplerBindingType_Filtering;

        // Create a bind group layout
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.nextInChain = nullptr;
        bindGroupLayoutDesc.entryCount = layouts.size();
        bindGroupLayoutDesc.entries = layouts.data();
        cubemapBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
    }

    void configureSurface(int width, int height) {
        WGPUSurfaceConfiguration config{};
        config.nextInChain = nullptr;

        config.width = width;
        config.height = height;

        surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
        config.format = surfaceFormat;
        config.viewFormatCount = 0;
        config.viewFormats = nullptr;

        config.presentMode = getBestPresentMode();
        config.alphaMode = WGPUCompositeAlphaMode_Auto;

        config.usage = WGPUTextureUsage_RenderAttachment;
        config.device = device;

        wgpuSurfaceConfigure(surface, &config);
    }

    void createDepthTexture(int width, int height) {
        WGPUTextureDescriptor depthTextureDesc{};
        depthTextureDesc.dimension = WGPUTextureDimension_2D;
        depthTextureDesc.format = depthFormat;
        depthTextureDesc.mipLevelCount = 1;
        depthTextureDesc.sampleCount = 1;
        depthTextureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
        depthTextureDesc.viewFormatCount = 1;
        depthTextureDesc.viewFormats = &depthFormat;
        depthTexture = wgpuDeviceCreateTexture(device, &depthTextureDesc);

        WGPUTextureViewDescriptor depthTextureViewDesc{};
        depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
        depthTextureViewDesc.baseArrayLayer = 0;
        depthTextureViewDesc.arrayLayerCount = 1;
        depthTextureViewDesc.baseMipLevel = 0;
        depthTextureViewDesc.mipLevelCount = 1;
        depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
        depthTextureViewDesc.format = depthFormat;
        depthTextureView = wgpuTextureCreateView(depthTexture, &depthTextureViewDesc);
    }

    void createSwapchain(int width, int height) {
        configureSurface(width, height);
        createDepthTexture(width, height);
    }

    void destroySwapchain() {
        wgpuTextureViewRelease(depthTextureView);
        wgpuTextureDestroy(depthTexture);
        wgpuTextureRelease(depthTexture);
        wgpuSurfaceUnconfigure(surface);
    }

    void init(GLFWwindow **p, const char* n, int w, int h, GraphicsSettings s) {
        window = p;
        settings = s;

        initGLFW(w, h, n);
        int fbw, fbh;
        glfwGetFramebufferSize(*window, &fbw, &fbh);
        createInstance();
        createSurface();
        createAdapter();
        createDevice();
        createUniformBindGroupLayout();
        createTextureBindGroupLayout();
        createCubemapBindGroupLayout();
        createCommandObjects();
        createSwapchain(fbw, fbh);
        pushConstantBufferID = JE::GFX::createUniformBuffer(pushConstantBufferSize * 2);

        ImGui::CreateContext();
        ImGui::GetIO();

        ImGui_ImplGlfw_InitForOther(*window, true);
        ImGui_ImplWGPU_InitInfo initInfo{};
        initInfo.Device = device;
        initInfo.NumFramesInFlight = 1;
        initInfo.RenderTargetFormat = surfaceFormat;
        initInfo.DepthStencilFormat = depthFormat;
        ImGui_ImplWGPU_Init(&initInfo);
        ImGui_ImplWGPU_CreateDeviceObjects();
    }

    WGPUTextureView acquireNext() {
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) return nullptr;

        WGPUTextureViewDescriptor viewDesc;
        viewDesc.nextInChain = nullptr;
        viewDesc.label = "John Descriptor"; // Please stop letting me name things
        viewDesc.format = wgpuTextureGetFormat(surfaceTexture.texture);
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;

        auto view = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);
#ifndef WEBGPU_BACKEND_WGPU // If we all just complied to real browser implementation life could be dream
        wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU
        return view;
    }

    void renderFrame(const std::vector<Renderable*>& renderables, const std::vector<void (*)()>& imGuiCalls) {
        WGPUTextureView next = acquireNext();
        if (!next) return;

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplWGPU_NewFrame();

        ImGui::NewFrame();
        for (auto fn : imGuiCalls) {
            fn();
        }
        ImGui::EndFrame();
        ImGui::Render();

        size_t requiredPushConstSize = renderables.size() * (sizeof(glm::mat4) * 2);
        if (requiredPushConstSize > pushConstantBufferSize) {
            pushConstantBufferSize = pushConstantBufferSize * 2;
            if (requiredPushConstSize > pushConstantBufferSize)
                pushConstantBufferSize = requiredPushConstSize;
            resizePushConstantBuffer(pushConstantBufferSize * 2);
        }

        // I swear the Vulkan PTSD is fully set in
        WGPUCommandEncoderDescriptor encoderDesc{};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "JEframecommandencoder";
        auto encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);


        WGPURenderPassColorAttachment renderPassColorAttachment{};
        renderPassColorAttachment.nextInChain = nullptr;

        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = {settings.clearColor[0], settings.clearColor[1], settings.clearColor[2], 1.0f};

        renderPassColorAttachment.view = next;
        renderPassColorAttachment.resolveTarget = nullptr;

#ifndef WEBGPU_BACKEND_WGPU
        renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

        WGPURenderPassDepthStencilAttachment depthStencilAttachment;
        depthStencilAttachment.view = depthTextureView;

        // Feels right at home...
        depthStencilAttachment.depthClearValue = 1.0f;
        depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
        depthStencilAttachment.depthReadOnly = false;

        // TODO: At some point in both APIs, maybe add support for this?
        depthStencilAttachment.stencilClearValue = 0;
        depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear; // Where's LOAD_OP_DONT_CARE when you need it?
        depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
        depthStencilAttachment.stencilReadOnly = true;

        WGPURenderPassDescriptor passDesc{};
        passDesc.nextInChain = nullptr;
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &renderPassColorAttachment;
        passDesc.depthStencilAttachment =  &depthStencilAttachment;
        passDesc.timestampWrites = nullptr;

        std::vector<PushConstants> constants{};
        // Generate render commands
        auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);

        uint32_t dynamicOffset = 0;
        uint32_t badWorkaround = 0;
        for (auto r : renderables) {
            wgpuRenderPassEncoderSetPipeline(pass, pipelineVector[r->shaderProgram]);

            size_t s = wgpuBufferGetSize(buffers[r->vboID]);
            // Set vertex buffer while encoding the render pass
            wgpuRenderPassEncoderSetVertexBuffer(pass, 0, buffers[r->vboID], 0, s);
            wgpuRenderPassEncoderSetIndexBuffer(pass, buffers[r->vboID + 1], WGPUIndexFormat_Uint32, 0,
                                                wgpuBufferGetSize(buffers[r->vboID + 1]));


            // Push Constant hacky solution
            wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroups[indexPairs[pushConstantBufferID].bindGroup], 1, &dynamicOffset);
            constants.push_back({r->objectMatrix, r->data});
            constants.push_back({});
            dynamicOffset += sizeof(PushConstants)*2;

            for (int i = 0; i < r->descriptorIDs.size(); i++) {
                auto pair = indexPairs[r->descriptorIDs[i]];
                if (pair.isBuffer) {
                    wgpuRenderPassEncoderSetBindGroup(pass,
                                                      i + 1,
                                                      bindGroups[pair.bindGroup],
                                                      1, &badWorkaround);
                } else {
                    wgpuRenderPassEncoderSetBindGroup(pass,
                                                      i + 1,
                                                      bindGroups[pair.bindGroup],
                                                      0, nullptr);
                }
            }

            // Draw 1 instance of a 3-vertices shape
            wgpuRenderPassEncoderDrawIndexed(pass, r->indicesSize, 1, 0, 0, 0);
        }

        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        // Submit render commands
        WGPUCommandBufferDescriptor commandBufferDesc{};
        commandBufferDesc.nextInChain = nullptr;
        commandBufferDesc.label = "Render Command Buffer";
        auto command = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
        wgpuCommandEncoderRelease(encoder);

        updateUniformBuffer(pushConstantBufferID, constants.data(), constants.size()*sizeof(PushConstants), false);
        wgpuQueueSubmit(queue, 1, &command);
        wgpuCommandBufferRelease(command);

        wgpuTextureViewRelease(next);

        wgpuSurfacePresent(surface);
        wgpuDevicePoll(device, false, nullptr);
    }

    void deinit() {
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplWGPU_Shutdown();

        for (auto bindGroup : bindGroups) {
            wgpuBindGroupRelease(bindGroup);
        }
        for (auto sampler : samplers) {
            wgpuSamplerRelease(sampler);
        }
        for (auto view : textureViews) {
            wgpuTextureViewRelease(view);
        }
        for (auto texture : textures) {
            wgpuTextureDestroy(texture);
            wgpuTextureRelease(texture);
        }
        for (auto buf : buffers) {
            wgpuBufferDestroy(buf);
            wgpuBufferRelease(buf);
        }
        for (auto shader : shaderModuleVector) {
            wgpuShaderModuleRelease(shader);
        }
        for (auto pipeline : pipelineVector) {
            wgpuRenderPipelineRelease(pipeline);
        }
        destroySwapchain();
        wgpuSurfaceRelease(surface);
        wgpuQueueRelease(queue);
        wgpuBindGroupLayoutRelease(cubemapBindGroupLayout);
        wgpuBindGroupLayoutRelease(textureBindGroupLayout);
        wgpuBindGroupLayoutRelease(uniformBindGroupLayout);
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuSurfaceRelease(surface);
        wgpuInstanceRelease(instance);
        glfwDestroyWindow(*window);
        glfwTerminate();
    }

    void resizeViewport() {
        int width, height;
        glfwGetFramebufferSize(*window, &width, &height);

        destroySwapchain();
        createSwapchain(width, height);
    }

    void setClearColor(float r, float g, float b) {
        settings.clearColor[0] = r;
        settings.clearColor[1] = g;
        settings.clearColor[2] = b;
    }

    unsigned int loadTextureBytes(const void* src, unsigned int width, unsigned int height, const int& sampleFilter) {
        //TODO: Implement textures
        const unsigned int channels = 4;
        size_t size = width * height * channels;

        WGPUTextureDescriptor textureDesc{};
        textureDesc.nextInChain = nullptr;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size = {width, height, 1};
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
        textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        textureDesc.viewFormatCount = 0;
        textureDesc.viewFormats = nullptr;

        auto texture = wgpuDeviceCreateTexture(device, &textureDesc);
        if (texture == nullptr) throw std::runtime_error("WGPU: Failed to create texture!");

        WGPUImageCopyTexture destination{};
        destination.texture = texture;
        destination.mipLevel = 0;
        destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
        destination.aspect = WGPUTextureAspect_All; // only relevant for depth/Stencil textures

        WGPUTextureDataLayout dataLayout{};
        dataLayout.offset = 0;
        dataLayout.bytesPerRow = 4 * textureDesc.size.width;
        dataLayout.rowsPerImage = textureDesc.size.height;

        wgpuQueueWriteTexture(queue, &destination, src, size, &dataLayout, &textureDesc.size);

        WGPUTextureViewDescriptor textureViewDesc{};
        textureViewDesc.aspect = WGPUTextureAspect_All;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 1;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.dimension = WGPUTextureViewDimension_2D;
        textureViewDesc.format = textureDesc.format;
        auto textureView = wgpuTextureCreateView(texture, &textureViewDesc);
        if (textureView == nullptr) throw std::runtime_error("WGPU: Failed to create texture view!");

        WGPUSamplerDescriptor samplerDesc;
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = sampleFilter == 0 ? WGPUFilterMode_Nearest : WGPUFilterMode_Linear;
        samplerDesc.minFilter = sampleFilter == 0 ? WGPUFilterMode_Nearest : WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        samplerDesc.lodMinClamp = 0.0f;
        samplerDesc.lodMaxClamp = 1.0f;
        samplerDesc.compare = WGPUCompareFunction_Undefined;
        samplerDesc.maxAnisotropy = 1;
        WGPUSampler sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

        std::array<WGPUBindGroupEntry, 2> bindings{};
        bindings[0].binding = 0;
        bindings[0].textureView = textureView;

        bindings[1].binding = 1;
        bindings[1].sampler = sampler;

        WGPUBindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.layout = textureBindGroupLayout;
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries = bindings.data();
        auto bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

        if (bindGroup == nullptr) throw std::runtime_error("WGPU: Failed to create texture bind group!");
        unsigned int bindID = bindGroups.size();
        bindGroups.push_back(bindGroup);

        unsigned int textureID = textureViews.size();
        textureViews.push_back(textureView);
        textures.push_back(texture);
        samplers.push_back(sampler);

        unsigned int id = indexPairs.size();
        indexPairs.push_back({textureID, bindID, false});

        return id;
    }

    unsigned int loadTexture(const std::string& fileName, const int& samplerFilter) {
        stbi_set_flip_vertically_on_load(true);
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("WGPU: Failed to load image \"" + fileName + "\"!");
        }
        return loadTextureBytes(pixels, texWidth, texHeight, samplerFilter);
    }

    unsigned int loadBundledTexture(char* fileFirstBytePtr, size_t fileLength, const int& samplerFilter) {
        stbi_set_flip_vertically_on_load(true);
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(fileFirstBytePtr), fileLength,
                                                &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("WGPU: Failed to load image from bundle!");
        }
        return loadTextureBytes(pixels, texWidth, texHeight, samplerFilter);
    }

    unsigned int loadCubemap(std::vector<std::string> faces) {
        int texWidth[6], texHeight[6], texChannels[6];

        stbi_set_flip_vertically_on_load(false);

        stbi_uc *images[6];
        for (int i = 0; i < 6; i++) {
            images[i] = stbi_load(faces[i].c_str(), &texWidth[i], &texHeight[i], &texChannels[i], STBI_rgb_alpha);
        }

        size_t layerSize = texWidth[0] * texHeight[0] * 4;
        size_t totalSize = layerSize * 6;

        WGPUTextureDescriptor textureDesc{};
        textureDesc.nextInChain = nullptr;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size = {static_cast<uint32_t>(texWidth[0]), static_cast<uint32_t>(texHeight[0]), 6};
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;
        textureDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
        textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        textureDesc.viewFormatCount = 0;
        textureDesc.viewFormats = nullptr;

        auto texture = wgpuDeviceCreateTexture(device, &textureDesc);
        if (texture == nullptr) throw std::runtime_error("WGPU: Failed to create texture!");

        WGPUTextureDataLayout dataLayout{};
        dataLayout.offset = 0;
        dataLayout.bytesPerRow = 4 * textureDesc.size.width;
        dataLayout.rowsPerImage = textureDesc.size.height;
        WGPUExtent3D writeSize{ static_cast<uint32_t>(texWidth[0]), static_cast<uint32_t>(texHeight[0]), 1 };

        for (int i = 0; i < 6; i++) {
            WGPUImageCopyTexture destination{};
            destination.texture = texture;
            destination.mipLevel = 0;
            destination.origin = {0, 0, static_cast<uint32_t>(i)}; // equivalent of the offset argument of Queue::writeBuffer
            destination.aspect = WGPUTextureAspect_All; // only relevant for depth/Stencil textures

            wgpuQueueWriteTexture(queue, &destination, images[i], layerSize, &dataLayout, &writeSize);
        }

        WGPUTextureViewDescriptor textureViewDesc{};
        textureViewDesc.aspect = WGPUTextureAspect_All;
        textureViewDesc.baseArrayLayer = 0;
        textureViewDesc.arrayLayerCount = 6;
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.dimension = WGPUTextureViewDimension_Cube;
        textureViewDesc.format = textureDesc.format;
        auto textureView = wgpuTextureCreateView(texture, &textureViewDesc);
        if (textureView == nullptr) throw std::runtime_error("WGPU: Failed to create texture view!");

        WGPUSamplerDescriptor samplerDesc;
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = WGPUFilterMode_Linear;
        samplerDesc.minFilter = WGPUFilterMode_Linear;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        samplerDesc.lodMinClamp = 0.0f;
        samplerDesc.lodMaxClamp = 1.0f;
        samplerDesc.compare = WGPUCompareFunction_Undefined;
        samplerDesc.maxAnisotropy = 1;
        WGPUSampler sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

        std::array<WGPUBindGroupEntry, 2> bindings{};
        bindings[0].binding = 0;
        bindings[0].textureView = textureView;

        bindings[1].binding = 1;
        bindings[1].sampler = sampler;

        WGPUBindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.layout = cubemapBindGroupLayout;
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries = bindings.data();
        auto bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

        if (bindGroup == nullptr) throw std::runtime_error("WGPU: Failed to create texture bind group!");
        unsigned int bindID = bindGroups.size();
        bindGroups.push_back(bindGroup);

        unsigned int textureID = textureViews.size();
        textureViews.push_back(textureView);
        textures.push_back(texture);
        samplers.push_back(sampler);

        unsigned int id = indexPairs.size();
        indexPairs.push_back({textureID, bindID, false});

        return id;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("WGPU: Failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize+1); // weirdly needs null terminator (why doesn't this happen on vulkan?)
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
        std::string copy = file_path;
        if (ends_with(file_path, ".glsl")) {
            unsigned int lastSubdir = file_path.rfind('/');
            copy = file_path.substr(0, lastSubdir) + "/wgsl_conv/" +
                   file_path.substr(lastSubdir + 1, file_path.length() - 5 - (lastSubdir + 1)) + ".wgsl";
        }

        WGPUShaderModuleDescriptor shaderDesc{};
#ifdef WEBGPU_BACKEND_WGPU
        shaderDesc.hintCount = 0;
        shaderDesc.hints = nullptr;
#endif
        WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
        shaderCodeDesc.chain.next = nullptr;
        shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        auto code = readFile(copy);
        shaderCodeDesc.code = &code[0];

        shaderDesc.nextInChain = &shaderCodeDesc.chain;
        WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

        if (shaderModule == nullptr) {
            throw std::runtime_error("WGPU: Failed to create shader module for \"" + copy + "\"!");
        }

        unsigned int id = shaderModuleVector.size();
        shaderModuleVector.push_back(shaderModule);
        return id;
    }

    void setDefaultStencilFaceState(WGPUStencilFaceState &stencilFaceState) {
        stencilFaceState.compare = WGPUCompareFunction_Always;
        stencilFaceState.failOp = WGPUStencilOperation_Keep;
        stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
        stencilFaceState.passOp = WGPUStencilOperation_Keep;
    }

    void setDefaultDepthStencilState(WGPUDepthStencilState &depthStencilState) {
        depthStencilState.format = WGPUTextureFormat_Undefined;
        depthStencilState.depthWriteEnabled = false;
        depthStencilState.depthCompare = WGPUCompareFunction_Always;
        depthStencilState.stencilReadMask = 0xFFFFFFFF;
        depthStencilState.stencilWriteMask = 0xFFFFFFFF;
        depthStencilState.depthBias = 0;
        depthStencilState.depthBiasSlopeScale = 0;
        depthStencilState.depthBiasClamp = 0;
        setDefaultStencilFaceState(depthStencilState.stencilFront);
        setDefaultStencilFaceState(depthStencilState.stencilBack);
    }

    unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype, bool use2DSamplers) {
        WGPURenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.nextInChain = nullptr;

        pipelineDesc.vertex.bufferCount = 0;
        pipelineDesc.vertex.buffers = nullptr;

        std::vector<WGPUVertexAttribute> vtxAttrs{};
        vtxAttrs.push_back({});
        vtxAttrs[0].shaderLocation = 0;
        vtxAttrs[0].format = WGPUVertexFormat_Float32x3;
        vtxAttrs[0].offset = offsetof(InterleavedVertex, position);

        vtxAttrs.push_back({});
        vtxAttrs[1].shaderLocation = 1;
        vtxAttrs[1].format = WGPUVertexFormat_Float32x2;
        vtxAttrs[1].offset = offsetof(InterleavedVertex, uvCoords);

        vtxAttrs.push_back({});
        vtxAttrs[2].shaderLocation = 2;
        vtxAttrs[2].format = WGPUVertexFormat_Float32x3;
        vtxAttrs[2].offset = offsetof(InterleavedVertex, normal);

        if (vtype == ANIMATED_VERTEX) {
            vtxAttrs.push_back({});
            vtxAttrs[3].shaderLocation = 3;
            vtxAttrs[3].format = WGPUVertexFormat_Uint8x2;
            vtxAttrs[3].offset = offsetof(InterleavedAnimatedVertex, groupID);
        }

        WGPUVertexBufferLayout vertexBufferLayout{};
        vertexBufferLayout.attributeCount = vtxAttrs.size();
        vertexBufferLayout.attributes = &vtxAttrs[0];
        vertexBufferLayout.arrayStride = vtype == VERTEX ? sizeof(InterleavedVertex) : sizeof(InterleavedAnimatedVertex);
        vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

        pipelineDesc.vertex.module = shaderModuleVector[VertexShaderID];
        pipelineDesc.vertex.entryPoint = "main";
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;
        pipelineDesc.vertex.bufferCount = 1;
        pipelineDesc.vertex.buffers = &vertexBufferLayout;

        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDesc.primitive.cullMode = shaderProgramSettings.doubleSided ? WGPUCullMode_None : WGPUCullMode_Back;

        WGPUBlendState blendState{};
        if (shaderProgramSettings.transparencySupported) {
            blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
            blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
            blendState.color.operation = WGPUBlendOperation_Add;
        } else {
            blendState.color.srcFactor = WGPUBlendFactor_One;
            blendState.color.dstFactor = WGPUBlendFactor_Zero;
            blendState.color.operation = WGPUBlendOperation_Add;
        }
        blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_Add;

        WGPUColorTargetState colorTarget{};
        colorTarget.format = surfaceFormat;
        colorTarget.blend = &blendState;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragmentState{};
        fragmentState.module = shaderModuleVector[FragmentShaderID];
        fragmentState.entryPoint = "main";
        fragmentState.constantCount = 0;
        fragmentState.constants = nullptr;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        pipelineDesc.fragment = &fragmentState;

        WGPUDepthStencilState depthStencilState{};
        setDefaultDepthStencilState(depthStencilState);
        depthStencilState.nextInChain = nullptr;
        depthStencilState.format = depthFormat;
        depthStencilState.depthWriteEnabled = !shaderProgramSettings.transparencySupported;
        depthStencilState.depthCompare = shaderProgramSettings.depthAlwaysPass ? WGPUCompareFunction_Always
                                                                               : WGPUCompareFunction_LessEqual;
        depthStencilState.stencilReadMask = 0;
        depthStencilState.stencilWriteMask = 0;

        pipelineDesc.depthStencil = &depthStencilState;

        // TODO MSAA
        pipelineDesc.multisample.count = 1;
        pipelineDesc.multisample.mask = ~0u;
        pipelineDesc.multisample.alphaToCoverageEnabled = false;

        // Direct translation of "dsls" loop in gfx_vk.cpp
        std::vector<WGPUBindGroupLayout> bgls{};
        bgls.push_back(uniformBindGroupLayout);
        for (int i = 0; i < shaderProgramSettings.shaderInputCount; i++) {
            //  select single bit from shader inputs
            if (((shaderProgramSettings.shaderInputs >> i) & 0b1) == 1) {
                // texture
                bgls.push_back(use2DSamplers ? textureBindGroupLayout : cubemapBindGroupLayout);
            } else {
                // uniform
                bgls.push_back(uniformBindGroupLayout);
            }
        }

        // Create the pipeline layout
        WGPUPipelineLayoutDescriptor layoutDesc{};
        layoutDesc.nextInChain = nullptr;
        layoutDesc.bindGroupLayoutCount = bgls.size();
        layoutDesc.bindGroupLayouts = bgls.data();
        auto layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

        pipelineDesc.layout = layout;

        auto pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

        if (pipeline == nullptr) {
            throw std::runtime_error("WGPU: Failed to create graphics pipeline!");
        }

        unsigned int id = pipelineVector.size();
        pipelineVector.push_back(pipeline);
        return id;
    }

    unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype) {
        return createProgram(VertexShaderID, FragmentShaderID, shaderProgramSettings, vtype, true);
    }

    unsigned int createLifetimeBuffer(void* src, size_t size, WGPUBufferUsage usage, std::vector<WGPUBuffer>& vec, const char* label) {
        WGPUBufferDescriptor bufferDesc{};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = label;
        bufferDesc.usage = usage;
        bufferDesc.size = size;
        bufferDesc.mappedAtCreation = true;

        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        if (buffer == nullptr) throw std::runtime_error("WGPU: Failed to create lifetime buffer!");

        void* map = wgpuBufferGetMappedRange(buffer, 0, size);
        memcpy(map, src, size);
        wgpuBufferUnmap(buffer);

        unsigned int id = vec.size();
        vec.push_back(buffer);
        return id;
    }

    unsigned int createVBO(std::vector<InterleavedVertex> *interleavedVertices, std::vector<unsigned int> *indices) {
        unsigned int idx1 =
        createLifetimeBuffer(interleavedVertices->data(),
                             interleavedVertices->size()*sizeof(InterleavedVertex),
                             WGPUBufferUsage_Vertex,
                             buffers);
        unsigned int idx2 =
        createLifetimeBuffer(indices->data(),
                             indices->size()*sizeof(unsigned int),
                             WGPUBufferUsage_Index,
                             buffers);
        if (idx1+1 != idx2) throw std::runtime_error("WGPU: Vertex/Index buffer ID desynced!");
        return idx1;
    }

    unsigned int createUniformBuffer(size_t bufferSize) {
        // WebGPU quirk.
        bufferSize = (bufferSize - (bufferSize % 16)) + 16;

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = "JEuniformbuffer";
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        bufferDesc.size = bufferSize;
        bufferDesc.mappedAtCreation = false;
        auto buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

        if (buffer == nullptr) throw std::runtime_error("WGPU: Failed to create uniform buffer!");
        unsigned int bufferID = buffers.size();
        buffers.push_back(buffer);

        // Create a binding
        WGPUBindGroupEntry binding{};
        binding.nextInChain = nullptr;
        binding.binding = 0;
        binding.buffer = buffers[bufferID];
        binding.offset = 0;
        binding.size = bufferSize;

        WGPUBindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.layout = uniformBindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &binding;
        auto bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

        if (bindGroup == nullptr) throw std::runtime_error("WGPU: Failed to create uniform bind group!");
        unsigned int bindID = bindGroups.size();
        bindGroups.push_back(bindGroup);

        unsigned int id = indexPairs.size();
        indexPairs.push_back({bufferID, bindID, true});

        return id;
    }

    void resizePushConstantBuffer(size_t size) {
        auto pair = indexPairs[pushConstantBufferID];

        wgpuBufferDestroy(buffers[pair.resource]);
        wgpuBindGroupRelease(bindGroups[pair.bindGroup]);

        // WebGPU quirk.
        size = (size - (size % 16)) + 16;

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = "JEuniformbuffer";
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        bufferDesc.size = size;
        bufferDesc.mappedAtCreation = false;
        auto buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

        if (buffer == nullptr) throw std::runtime_error("WGPU: Failed to create uniform buffer!");
        buffers[pair.resource] = buffer;

        // Create a binding
        WGPUBindGroupEntry binding{};
        binding.nextInChain = nullptr;
        binding.binding = 0;
        binding.buffer = buffers[pair.resource];
        binding.offset = 0;
        binding.size = sizeof(PushConstants)*2;

        WGPUBindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.layout = uniformBindGroupLayout;
        bindGroupDesc.entryCount = 1;
        bindGroupDesc.entries = &binding;
        auto bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

        if (bindGroup == nullptr) throw std::runtime_error("WGPU: Failed to create uniform bind group!");
        bindGroups[pair.bindGroup] = bindGroup;
    }

    void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll) {
        wgpuQueueWriteBuffer(queue, buffers[indexPairs[id].resource], 0, ptr, size);
    }
}