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

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include "../../ui/imgui/imgui.h"
#include "../../ui/imgui/imgui_impl_glfw.h"
#include "../../ui/imgui/imgui_impl_wgpu.h"

namespace JE::GFX {

    unsigned int createLifetimeBuffer(void* src, size_t size, WGPUBufferUsage usage, std::vector<WGPUBuffer>& vec, const char* label = "JElifetimebuffer");

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

    WGPUBindGroupLayout uniformBindGroupLayout;
    WGPUBindGroupLayout textureBindGroupLayout;

    std::vector<WGPUBindGroup> bindGroups{};

    struct index_pair {unsigned int resource; unsigned int bindGroup;};
    std::vector<index_pair> indexPairs{};

    void initGLFW(int width, int height, const char* windowName) {
        if (!glfwInit()) {
            throw std::runtime_error("WGPU: Could not initialize GLFW!");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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

    // Boilerplate? I hardly know her!
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
    }

    void createDevice() {
        WGPUDeviceDescriptor deviceDescriptor;
        deviceDescriptor.label = "John Graphics"; // why do these things have names??
        deviceDescriptor.requiredFeatureCount = 0;
        //WGPURequiredLimits reqs = getRequiredLimits();
        //deviceDescriptor.requiredLimits = &reqs;
        deviceDescriptor.requiredLimits = nullptr; // Weirdly enough, the tutorial tells me to do this then doesn't...
        deviceDescriptor.defaultQueue.nextInChain = nullptr;
        deviceDescriptor.defaultQueue.label = "John Queue"; // what??
        deviceDescriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void*) {
            std::cout << "WGPU: Device lost,  " << reason;
            if (message) std::cout << " (" << message << ")";
            std::cout << std::endl;
        };;

        bool awty = false;

        wgpuAdapterRequestDevice(adapter, &deviceDescriptor, [](WGPURequestDeviceStatus status, WGPUDevice d, char const * message, void * b) {
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
        bindingLayout.visibility = WGPUShaderStage_Vertex;
        bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
        bindingLayout.buffer.minBindingSize = 2 * sizeof(uint64_t);

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

        setDefaultBinding(layouts[1]);
        layouts[1].binding = 1;
        layouts[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;

        // Create a bind group layout
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
        bindGroupLayoutDesc.nextInChain = nullptr;
        bindGroupLayoutDesc.entryCount = layouts.size();
        bindGroupLayoutDesc.entries = layouts.data();
        textureBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
    }

    void init(GLFWwindow **p, const char* n, int w, int h, GraphicsSettings s) {
        window = p;
        settings = s;

        initGLFW(w, h, n);
        createInstance();
        createSurface();
        createAdapter();
        createDevice();
        createUniformBindGroupLayout();
        //createTextureBindGroupLayout();
        createCommandObjects();
        configureSurface(w, h);

        //TODO remove
        unsigned int testv = loadShader("./shaders/temp_triangle_vert.wgsl", 0);
        unsigned int testf = loadShader("./shaders/temp_triangle_frag.wgsl", 0);
        ShaderProgramSettings shaderProgramSettings{};
        shaderProgramSettings.transparencySupported = false;
        shaderProgramSettings.doubleSided = true;
        shaderProgramSettings.shaderInputs = 0b0;
        shaderProgramSettings.shaderInputCount = 1;

        createProgram(testv, testf, shaderProgramSettings, VERTEX);

        std::vector<InterleavedVertex> vertexData = {
                // x0, y0
                {{-0.5, -0.5, 0.0}, {0.0, 0.0}, {0.0, 0.0, 1.0}},

                // x1, y1
                {{+0.5, -0.5, 0.0}, {1.0, 0.0}, {0.0, 0.0, 1.0}},

                // x2, y2
                {{+0.0, +0.5, 0.0}, {0.5, 1.0}, {0.0, 0.0, 1.0}}
        };

        std::vector<unsigned int> indexData = {0, 1, 2};

        createVBO(&vertexData, &indexData);
        unsigned int ubufid = JE::GFX::createUniformBuffer(sizeof(float)); // specify JE::GFX not JE
        float f = -0.5f;
        JE::GFX::updateUniformBuffer(ubufid, &f, sizeof(float), false);
    }

    WGPUTextureView acquireNext() {
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) return nullptr;

        WGPUTextureViewDescriptor viewDescriptor;
        viewDescriptor.nextInChain = nullptr;
        viewDescriptor.label = "John Descriptor"; // Please stop letting me name things
        viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
        viewDescriptor.dimension = WGPUTextureViewDimension_2D;
        viewDescriptor.baseMipLevel = 0;
        viewDescriptor.mipLevelCount = 1;
        viewDescriptor.baseArrayLayer = 0;
        viewDescriptor.arrayLayerCount = 1;
        viewDescriptor.aspect = WGPUTextureAspect_All;

        auto view = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
#ifndef WEBGPU_BACKEND_WGPU // If we all just complied to real browser implementation life could be dream
        wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU
        return view;
    }

    void renderFrame(const std::vector<Renderable*>& renderables, const std::vector<void (*)()>& imGuiCalls) {
        WGPUTextureView next = acquireNext();
        if (!next) return;

        // I swear the Vulkan PTSD is fully set in
        WGPUCommandEncoderDescriptor encoderDesc{};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "Frame Command Encoder";
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


        // Create pass descriptor
        WGPURenderPassDescriptor renderPassDescriptor{};
        renderPassDescriptor.nextInChain = nullptr;
        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
        renderPassDescriptor.depthStencilAttachment = nullptr;
        renderPassDescriptor.timestampWrites = nullptr;

        // Generate render commands
        auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);
        // TODO: Render triangles!
        // Select which render pipeline to use
        wgpuRenderPassEncoderSetPipeline(pass, pipelineVector[0]);

        unsigned int buf = 0;

        // Set vertex buffer while encoding the render pass
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, buffers[buf], 0, wgpuBufferGetSize(buffers[buf]));
        wgpuRenderPassEncoderSetIndexBuffer(pass, buffers[buf+1], WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(buffers[buf+1]));

        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroups[0], 0, nullptr);

        // Draw 1 instance of a 3-vertices shape
        wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

        wgpuRenderPassEncoderEnd(pass);
        wgpuRenderPassEncoderRelease(pass);

        auto t = static_cast<float>(sin(glfwGetTime())/2); // glfwGetTime returns a double
        updateUniformBuffer(0, &t, sizeof(float), false);

        // Submit render commands
        WGPUCommandBufferDescriptor commandBufferDescriptor{};
        commandBufferDescriptor.nextInChain = nullptr;
        commandBufferDescriptor.label = "Render Command Buffer";
        auto command = wgpuCommandEncoderFinish(encoder, &commandBufferDescriptor);
        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(queue, 1, &command);
        wgpuCommandBufferRelease(command);

        wgpuTextureViewRelease(next);

        wgpuSurfacePresent(surface);
        wgpuDevicePoll(device, false, nullptr);
    }

    void deinit() {
        for (auto bindGroup : bindGroups) {
            wgpuBindGroupRelease(bindGroup);
        }
        for (auto buf : buffers) {
            wgpuBufferRelease(buf);
        }
        for (auto shader : shaderModuleVector) {
            wgpuShaderModuleRelease(shader);
        }
        for (auto pipeline : pipelineVector) {
            wgpuRenderPipelineRelease(pipeline);
        }
        wgpuSurfaceUnconfigure(surface);
        wgpuQueueRelease(queue);
        //wgpuBindGroupLayoutRelease(textureBindGroupLayout);
        wgpuBindGroupLayoutRelease(uniformBindGroupLayout);
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuSurfaceRelease(surface);
        wgpuInstanceRelease(instance);
        glfwDestroyWindow(*window);
        glfwTerminate();
    }

    void resizeViewport() {

    }

    void setClearColor(float r, float g, float b) {
        settings.clearColor[0] = r;
        settings.clearColor[1] = g;
        settings.clearColor[2] = b;
    }

    unsigned int loadTexture(const std::string& fileName, const int& samplerFilter) {
        // TODO: Implement textures
        return 0;
    }

    unsigned int loadBundledTexture(char* fileFirstBytePtr, size_t fileLength, const int& samplerFilter) {
        // TODO: Implement textures
        return 0;
    }

    unsigned int loadCubemap(std::vector<std::string> faces) {
        // TODO: Implement textures
        return 0;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("WGPU: Failed to open file!");
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
        WGPUShaderModuleDescriptor shaderDesc{};
#ifdef WEBGPU_BACKEND_WGPU
        shaderDesc.hintCount = 0;
        shaderDesc.hints = nullptr;
#endif
        WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
        shaderCodeDesc.chain.next = nullptr;
        shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        auto code = readFile(file_path);
        shaderCodeDesc.code = &code[0];

        shaderDesc.nextInChain = &shaderCodeDesc.chain;
        WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

        if (shaderModule == nullptr) {
            throw std::runtime_error("WGPU: Failed to create shader module for \"" + file_path + "\"!");
        }

        unsigned int id = shaderModuleVector.size();
        shaderModuleVector.push_back(shaderModule);
        return id;
    }

    unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype) {
        WGPURenderPipelineDescriptor pipelineDescriptor{};
        pipelineDescriptor.nextInChain = nullptr;

        pipelineDescriptor.vertex.bufferCount = 0;
        pipelineDescriptor.vertex.buffers = nullptr;

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

        pipelineDescriptor.vertex.module = shaderModuleVector[VertexShaderID];
        pipelineDescriptor.vertex.entryPoint = "main";
        pipelineDescriptor.vertex.constantCount = 0;
        pipelineDescriptor.vertex.constants = nullptr;
        pipelineDescriptor.vertex.bufferCount = 1;
        pipelineDescriptor.vertex.buffers = &vertexBufferLayout;

        pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDescriptor.primitive.cullMode = shaderProgramSettings.doubleSided ? WGPUCullMode_None : WGPUCullMode_Back;

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

        pipelineDescriptor.fragment = &fragmentState;

        pipelineDescriptor.depthStencil = nullptr;

        // TODO MSAA
        pipelineDescriptor.multisample.count = 1;
        pipelineDescriptor.multisample.mask = ~0u;
        pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

        // Direct translation of "dsls" loop in gfx_vk.cpp
        std::vector<WGPUBindGroupLayout> bgls{};
        for (int i = 0; i < shaderProgramSettings.shaderInputCount; i++) {
            //  select single bit from shader inputs
            if (((shaderProgramSettings.shaderInputs >> i) & 0b1) == 1) {
                // texture
                bgls.push_back(textureBindGroupLayout);
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

        pipelineDescriptor.layout = layout;

        auto pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor);

        if (pipeline == nullptr) {
            throw std::runtime_error("WGPU: Failed to create graphics pipeline!");
        }

        unsigned int id = pipelineVector.size();
        pipelineVector.push_back(pipeline);
        return id;
    }

    unsigned int createLifetimeBuffer(void* src, size_t size, WGPUBufferUsage usage, std::vector<WGPUBuffer>& vec, const char* label) {
        WGPUBufferDescriptor bufferDescriptor{};
        bufferDescriptor.nextInChain = nullptr;
        bufferDescriptor.label = label;
        bufferDescriptor.usage = usage;
        bufferDescriptor.size = size;
        bufferDescriptor.mappedAtCreation = true;

        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDescriptor);
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
        createLifetimeBuffer(&(*interleavedVertices)[0],
                             interleavedVertices->size()*sizeof(InterleavedVertex),
                             WGPUBufferUsage_Vertex,
                             buffers);
        unsigned int idx2 =
        createLifetimeBuffer(&(*indices)[0],
                             indices->size()*sizeof(unsigned int),
                             WGPUBufferUsage_Index,
                             buffers);
        if (idx1+1 != idx2) throw std::runtime_error("WGPU: Vertex/Index buffer ID desynced!");
        return idx1;
    }

    unsigned int createUniformBuffer(size_t bufferSize) {
        // WebGPU quirk.
        if (bufferSize < 16) bufferSize = 16;

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
        indexPairs.push_back({bufferID, bindID});

        return id;
    }

    void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll) {
        wgpuQueueWriteBuffer(queue, buffers[indexPairs[id].resource], 0, ptr, size);
    }
}