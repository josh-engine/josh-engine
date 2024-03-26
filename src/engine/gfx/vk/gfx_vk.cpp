#include "../../engineconfig.h"
#ifdef GFX_API_VK
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "gfx_vk.h"
#include <map>
#include <set>

glm::vec3 ambient(glm::max(AMBIENT_RED - 0.5f, 0.1f), glm::max(AMBIENT_GREEN - 0.5f, 0.1f), glm::max(AMBIENT_BLUE - 0.5f, 0.1f));

GLFWwindow** windowPtr;

VkInstance instance;

VkPhysicalDevice physicalDevice;
VkDevice logicalDevice;

VkSurfaceKHR windowSurface;

VkQueue graphicsQueue;
VkQueue presentQueue;

VkSwapchainKHR swapChain;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char*> instanceExtensions = {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        "VK_KHR_get_physical_device_properties2"
};

const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
};

#ifndef DEBUG_ENABLED
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void initGLFW(){
    if (!glfwInit()){
        throw std::runtime_error("Vulkan: Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //TODO: Come back later to change resizable out
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    *windowPtr = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
}


bool platformSupportsValidationLayers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        // Jesus christ why are vulkan object names so LONG what the HECC
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Vulkan Validation Layer: " << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cout << "\e[0;33mVulkan Validation Layer: " << pCallbackData->pMessage << "\e[0m" << std::endl;
    } else {
        std::cout << "\e[0;37mVulkan Validation Layer: " << pCallbackData->pMessage << "\e[0m" << std::endl;
    }

    return VK_FALSE;
}

void createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = WINDOW_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "JoshEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (enableValidationLayers && !platformSupportsValidationLayers()){
        throw std::runtime_error("Vulkan: Debug enabled, but platform does not support validation layers!");
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> requiredExtensions = instanceExtensions;

    for(uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.emplace_back(glfwExtensions[i]);
    }

    if (enableValidationLayers) {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Could not create instance!");
    }
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, windowSurface, &presentSupport);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, windowSurface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(*windowPtr, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

int scoreDevice(VkPhysicalDevice device) {
    //get the stuff
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // dgpu = bueno
    score += ((int)(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))*1000;

    // big textures go brrrr
    score += deviceProperties.limits.maxImageDimension2D/25;

    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainSupportAdequate;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainSupportAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    if (indices.isComplete() && extensionsSupported && swapChainSupportAdequate) {
        return score;
    } else {
        return 0;
    }
}

void choosePhysicalDevice(){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
#ifdef DEBUG_ENABLED
        throw std::runtime_error("Vulkan: Failed to find GPUs with Vulkan support! Either gfx_vk.cpp is screwed up, or your computer doesn't have Vulkan support.");
#else
        throw std::runtime_error("Vulkan: Failed to find GPUs with Vulkan support! If you are a user reading this error, please tell the developer to compile a version with GFX_API_OPENGL33 as the Graphics API flag in src/engine/engineconfig.h.");
#endif
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::map<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = scoreDevice(device);
        candidates.insert({score, device});
    }
    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
#ifdef DEBUG_ENABLED
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        std::cout << "\e[0;37mVulkan: Selected " << deviceProperties.deviceName << (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? " (discrete GPU)" : " (iGPU)") << " as the chosen physical device with score " << candidates.rbegin()->first << ".\e[0m" << std::endl;
#endif //DEBUG_ENABLED
    } else {
        throw std::runtime_error("Vulkan: Failed to find a suitable GPU!");
    }
}

void createLogicalDevice(){
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Compat with old Vulkan implementations. Not actually required in recent.
    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = validationLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create logical device!");
    }

    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

void createSurface(){
    if (glfwCreateWindowSurface(instance, *windowPtr, nullptr, &windowSurface) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create window surface!");
    }
}

void createSwapchain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = windowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create swap chain!");
    }
}

void initGFX(GLFWwindow** window){
    windowPtr = window;
    initGLFW();
    createInstance();
    createSurface();
    choosePhysicalDevice();
    createLogicalDevice();
    createSwapchain();
}

unsigned int loadCubemap(std::vector<std::string> faces) {

}

unsigned int loadTexture(std::string fileName){

}

GLuint loadShader(const std::string file_path, int target){

}

GLuint createProgram(GLuint VertexShaderID, GLuint FragmentShaderID){

}

void renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, glm::vec3 camerapos, glm::vec3 cameradir, float fieldOfViewAngle, std::vector<Renderable> renderables, int w, int h, std::vector<void (*)()> imGuiCalls) {

}

void deinitGFX(GLFWwindow** window){
    vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
    vkDestroyDevice(logicalDevice, nullptr);
    vkDestroySurfaceKHR(instance, windowSurface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(*window);
    glfwTerminate();
}
#endif //GFX_API_VK