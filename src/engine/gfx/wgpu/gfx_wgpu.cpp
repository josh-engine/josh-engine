//
// Created by Ethan Lee on 11/27/24.
//

#include "gfx_wgpu.h"
#include <iostream>
#include <vector>
#include <cassert>

#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

namespace JE::GFX {
    GLFWwindow** window;
    float clear[3];

    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;

    WGPUQueue queue;
    WGPUCommandEncoder encoder;

    WGPUSurface surface;

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
        WGPUInstanceDescriptor desc = {};
        desc.nextInChain = nullptr;

        instance = wgpuCreateInstance(&desc);
#endif

        if (!instance) {
            throw std::runtime_error("WGPU: Could not initialize!");
        }
    }

    void createAdapter() {
        WGPURequestAdapterOptions adapterOpts = {};
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

    void createDevice() {
        WGPUDeviceDescriptor deviceDescriptor;
        deviceDescriptor.label = "John Graphics"; // why do these things have names??
        deviceDescriptor.requiredFeatureCount = 0;
        deviceDescriptor.requiredLimits = nullptr;
        deviceDescriptor.defaultQueue.nextInChain = nullptr;
        deviceDescriptor.defaultQueue.label = "John Queue"; // what??
        deviceDescriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
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

        // I swear the Vulkan PTSD is fully set in
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "JE Command Encoder";
        encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
    }

    void createSurface() {
        surface = glfwGetWGPUSurface(instance, *window);

        if (surface == nullptr) {
            throw std::runtime_error("WGPU: Failed to get surface!");
        }
    }

    void init(GLFWwindow **wptr, const char* windowName, int width, int height, GraphicsSettings settings) {
        window = wptr;
        memcpy(clear, settings.clearColor, sizeof(float)*3);

        initGLFW(width, height, windowName);
        createInstance();
        createSurface();
        createAdapter();
        createDevice();
        createCommandObjects();

    }

    void renderFrame(const std::vector<Renderable*>& renderables, const std::vector<void (*)()>& imGuiCalls) {
        wgpuDevicePoll(device, false, nullptr);
    }

    void deinit() {
        wgpuCommandEncoderRelease(encoder);
        wgpuQueueRelease(queue);
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

    unsigned int loadShader(const std::string& file_path, int target) {
        // TODO: Implement shaders
        return 0;
    }

    unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, const ShaderProgramSettings& shaderProgramSettings, VertexType vtype) {
        // TODO: Implement shaders
        return 0;
    }

    unsigned int createVBO(std::vector<InterleavedVertex> *interleavedVertices, std::vector<unsigned int> *indices) {
        // TODO: Implement VBOs
        return 0;
    }

    unsigned int createUniformBuffer(size_t bufferSize) {
        // TODO: Implement uniform buffers
        return 0;
    }
    void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll) {
        // TODO: Implement uniform buffers
    }
}