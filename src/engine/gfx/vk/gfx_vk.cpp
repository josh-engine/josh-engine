/*
 * Created by Ember Lee on 3/26/24
 * (Mostly) finished by Ember Lee on 3/29/24
 * Implementation based on vulkan-tutorial.org.
 * I didn't look at a lot of reference material online, so there is definitely bound to be some problems.
 * Final total of lines in this file: 1969 (1981 minus 12 for this comment block)
 * OpenGL is still faster but hey at least I tried.
 * Fix anything here if you'd like but please don't break OpenGL shader compat
 * (and don't increase the OpenGL version again unless writing a new backend, please.)
 * (that was in desperation and mac only supports up to 4.1.)
 */

#ifdef GFX_API_VK
#include "../../engineconfig.h"
#include "gfx_vk.h"

#include "../../ui/imgui/imgui_impl_glfw.h"
#include "../../ui/imgui/imgui_impl_vulkan.h"

#include <optional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <set>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../spirv/spirv-helper.h"

namespace JE::GFX {
    GLFWwindow **windowPtr;
    GraphicsSettings settings;

    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    VkSurfaceKHR windowSurface;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    std::vector<VkImageView> swapchainImageViews;

// This is a system to get the same "ID" concept working as with OpenGL.
    std::vector<VkShaderModule> shaderModuleVector;

    VkRenderPass renderPass;

// Same idea as the ID concept but with Pipelines roughly equating to Shader Programs
    std::vector<VkPipeline> pipelineVector;
    std::vector<VkPipelineLayout> pipelineLayoutVector;

    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkCommandPool commandPool;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;

    // Same ID system implementation.
    std::vector<VkBuffer> vertexBuffers;
    std::vector<Allocation> vertexBufferMemoryRefs;
    std::vector<size_t> vertexBufferSizes;
    std::vector<VkBuffer> indexBuffers;
    std::vector<Allocation> indexBufferMemoryRefs;
    std::vector<size_t> indexBufferSizes;

    VkDescriptorSetLayout uniformDescriptorSetLayout;
    VkDescriptorSetLayout textureDescriptorSetLayout;

    std::vector<std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT>> uniformBuffers;
    std::vector<std::array<Allocation, MAX_FRAMES_IN_FLIGHT>> uniformBuffersMemory;
    std::vector<std::array<void *, MAX_FRAMES_IN_FLIGHT>> uniformBuffersMapped;
    std::vector<VkDescriptorPool> uniformDescriptorPools;

    std::vector<VkImage> textureImages;
    std::vector<unsigned int> textureMipLevels;
    std::vector<Allocation> textureMemoryRefs;
    std::vector<VkImageView> textureImageViews;
    std::vector<VkDescriptorPool> textureDescriptorPools;
    std::vector<VkSampler> textureSamplers;

    std::vector<DescriptorSet> descriptorSets;

    VkDescriptorPool imGuiDescriptorPool;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

// Our framebuffer is large enough to justify a separate VkDeviceMemory
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<MemoryBlock> memoryBlocks{};

#ifdef DEBUG_ENABLED

    std::vector<MemoryBlock> getMemory() {
        return memoryBlocks;
    }

    void *getTex(const unsigned int i) {
        return descriptorSets[i].sets[0];
    }

    unsigned int getBufCount() {
        return uniformBuffers.size();
    }

    UniformBufferReference getBuf(const unsigned int i) {
        return {&uniformBuffersMemory[i], &(uniformBuffersMapped[i])};
    }

#endif

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const std::vector<const char*> instanceExtensions = {
#ifdef PLATFORM_MAC
            "VK_KHR_portability_enumeration"
#endif
    };

    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef PLATFORM_MAC
            "VK_KHR_portability_subset"
#endif
    };

    bool framebufferResized = false;

    void resizeViewport() {
        framebufferResized = true;
    }

    VkSampleCountFlagBits getMaxSamples(const auto vkPhysicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(vkPhysicalDevice, &physicalDeviceProperties);

        const auto counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                    physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkSampleCountFlagBits getClosestSampleCount(const auto vkPhysicalDevice) {
        if (settings.msaaSamples > 1) {
            const auto maxSamples = getMaxSamples(vkPhysicalDevice);
            if (maxSamples > settings.msaaSamples) {
                return static_cast<VkSampleCountFlagBits>(settings.msaaSamples);
            }
            return static_cast<VkSampleCountFlagBits>(maxSamples);
        }
        return VK_SAMPLE_COUNT_1_BIT;
    }

    void initGLFW(const char *name, const auto width, const auto height) {
        if (!glfwInit()) {
            throw std::runtime_error("Vulkan: Could not initialize GLFW!");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        *windowPtr = glfwCreateWindow(width, height, name, nullptr, nullptr);
    }


    uint32_t findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        // simulate not having any lazily allocated memory type (noah bug fix 3 attempt 1 testing)
        // if (properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) throw std::runtime_error("test");

        // In case we can't find device-local memory.
        int backupIndex = -1;

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                if (memProperties.memoryHeaps[memProperties.memoryTypes[i].heapIndex].flags &
                    VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    // Great! It's on the device.
                    return i;
                } else {
                    // Fine, we can default to this if nothing better is found.
                    backupIndex = static_cast<int>(i);
                }
            }
        }

        if (backupIndex != -1) return static_cast<uint32_t>(backupIndex);

        throw std::runtime_error("Vulkan: Failed to find suitable memory type!");
    }

    void allocateDeviceMemory(VkDeviceMemory &memory, const VkDeviceSize size, const uint32_t memoryType) {
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = size;
        allocInfo.memoryTypeIndex = memoryType;

        if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to allocate memory!");
        }
    }

    Allocation vkalloc(const auto size, const auto memoryType, const auto align, const auto willMap) {
        unsigned int i = 0;
        for (auto &block: memoryBlocks) {
            // is it the right type?
            if (block.type == memoryType) {
                // align the block top to whatever vulkan says we should.
                //           get to a lower multiple of align, add it again to get back up
                const auto alignedTop = block.top - block.top % align + align;
                // can we fit and do we have a map conflict?
                //                                      if it will map and the block already is, we can't use it
                if (block.size >= alignedTop + size && !(willMap && block.mapped)) {
                    block.top = alignedTop + size;
                    return {i, size, alignedTop};
                }
            }
            i++;
        }

        VkDeviceMemory newMemory;
        VkDeviceSize newBlockSize = (willMap ? size : NEW_BLOCK_MIN_SIZE);
        // do the vector thing and be prepared for double the size of our original data (if it is larger than our new min size)
        if (size * 2 > newBlockSize && !willMap) newBlockSize = size * 2;
        allocateDeviceMemory(newMemory, newBlockSize, memoryType);
        memoryBlocks.push_back({newMemory, memoryType, newBlockSize, size});
        return {static_cast<unsigned int>(memoryBlocks.size() - 1), size, 0};
    }

    Allocation vkalloc(const auto size, const auto memoryType, const auto align) {
        return vkalloc(size, memoryType, align, false);
    }

    void vkmmap(const Allocation *alloc, void **toMap) {
        vkMapMemory(logicalDevice, memoryBlocks[alloc->memoryRefID].memory, alloc->offset, alloc->size, 0, toMap);
    }

    void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      Allocation &alloc, const bool willMap) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

        alloc = vkalloc(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties),
                        memRequirements.alignment, willMap);

        vkBindBufferMemory(logicalDevice, buffer, memoryBlocks[alloc.memoryRefID].memory, alloc.offset);
    }

    void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer &buffer,
                      VkDeviceMemory &memory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);
        allocateDeviceMemory(memory, size, findMemoryType(memRequirements.memoryTypeBits, properties));
        vkBindBufferMemory(logicalDevice, buffer, memory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(const auto commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(const auto srcBuffer, const auto dstBuffer, const VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    bool platformSupportsValidationLayers() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: validationLayers) {
            bool layerFound = false;

            for (const auto &layerProperties: availableLayers) {
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
            const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            [[maybe_unused]] void *pUserData) {

        if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "Vulkan Validation Layer: " << pCallbackData->pMessage << std::endl;
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cout << "\x1b[0;33mVulkan Validation Layer: " << pCallbackData->pMessage << "\x1b[0m" << std::endl;
        } else {
            std::cout << "\x1b[0;37mVulkan Validation Layer: " << pCallbackData->pMessage << "\x1b[0m" << std::endl;
        }

        return VK_FALSE;
    }

    void createInstance(const char *name) {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = name;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "JoshEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

#ifdef DEBUG_ENABLED
        if (!platformSupportsValidationLayers()) {
            throw std::runtime_error("Vulkan: Debug enabled, but platform does not support validation layers!");
        }
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        createInfo.pNext = &debugCreateInfo;
#else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
#endif

        uint32_t glfwExtensionCount = 0;

        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> requiredExtensions = instanceExtensions;

        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            requiredExtensions.emplace_back(glfwExtensions[i]);
        }

#ifdef DEBUG_ENABLED
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#ifdef PLATFORM_MAC
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        createInfo.enabledExtensionCount = requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Could not create instance!");
        }
    }

    // ReSharper disable once CppParameterMayBeConst
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
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

    // ReSharper disable once CppParameterMayBeConst
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &[extensionName, specVersion]: availableExtensions) {
            requiredExtensions.erase(extensionName);
        }

        return requiredExtensions.empty();
    }

    // ReSharper disable once CppParameterMayBeConst
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
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount,
                                                      details.presentModes.data());
        }

        return details;
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode: availablePresentModes) {
            if (settings.vsyncEnabled) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            } else {
                if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    return availablePresentMode;
                }
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(*windowPtr, &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };

            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    int scoreDevice(VkPhysicalDevice device) {
        // get the stuff
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        // dgpu = bueno
        score += (static_cast<int>(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) * 1000;

        // big textures go brrrr
        // We will likely never hit the texture limit of modern GPUs, this scoring is BS
        //score += static_cast<int>(deviceProperties.limits.maxImageDimension2D/25);

        // higher msaa = bueno
        score += static_cast<int>(getMaxSamples(device) * 10);

        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapchainSupportAdequate;
        if (extensionsSupported) {
            SwapChainSupportDetails swapchainSupport = querySwapChainSupport(device);
            swapchainSupportAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        if (indices.isComplete() && extensionsSupported && swapchainSupportAdequate &&
            supportedFeatures.samplerAnisotropy) {
            return score;
        } else {
            return 0;
        }
    }

    void choosePhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
#ifdef DEBUG_ENABLED
            throw std::runtime_error(
                    "Vulkan: Failed to find GPUs with Vulkan support! Either gfx_vk.cpp is screwed up, or your computer doesn't have Vulkan support.");
#else
            throw std::runtime_error("Vulkan: Failed to find GPUs with Vulkan support! Your computer may not support Vulkan.");
#endif
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::priority_queue<std::pair<int, VkPhysicalDevice>, std::deque<std::pair<int, VkPhysicalDevice>>,
        decltype([](const auto &left, const auto &right) {
            return left.first < right.first;
        })> candidates;

        for (const auto &device: devices) {
            int score = scoreDevice(device);
            candidates.emplace(score, device);
        }

        // Check if the best candidate is suitable at all
        if (candidates.top().first > 0) {
            physicalDevice = candidates.top().second;
            msaaSamples = getClosestSampleCount(physicalDevice);
#ifdef DEBUG_ENABLED
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
            std::cout << "\e[0;37mVulkan: Selected " << deviceProperties.deviceName
                      << (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? " (discrete GPU)"
                                                                                              : " (iGPU)")
                      << " as the chosen physical device with score " << candidates.top().first << ".\e[0m"
                      << std::endl;
#endif //DEBUG_ENABLED
        } else {
            throw std::runtime_error("Vulkan: Failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set uniqueQueueFamilies = {graphicsFamily.value(), presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // Compat with old Vulkan implementations. Not actually required in recent.
#ifdef DEBUG_ENABLED
        deviceCreateInfo.enabledLayerCount = validationLayers.size();
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif

        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create logical device!");
        }

        vkGetDeviceQueue(logicalDevice, graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, presentFamily.value(), 0, &presentQueue);
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, *windowPtr, nullptr, &windowSurface) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create window surface!");
        }
    }

    void createSwapchain() {
        const auto [capabilities, formats, presentModes] = querySwapChainSupport(physicalDevice);

        const auto [format, colorSpace] = chooseSwapSurfaceFormat(formats);
        const auto presentMode = chooseSwapPresentMode(presentModes);
        const auto extent = chooseSwapExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;

        if (capabilities.maxImageCount > 0 &&
            imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = windowSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice);
        const uint32_t queueFamilyIndices[] = {graphicsFamily.value(), presentFamily.value()};

        if (graphicsFamily != presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapchainImages.data());

        swapchainImageFormat = format;
        swapchainExtent = extent;
    }

    VkImageView
    createImageView(const auto image, const auto format, const auto aspectFlag, const unsigned int mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlag;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create texture image view!");
        }

        return imageView;
    }

    void createImageViews() {
        swapchainImageViews.resize(swapchainImages.size());

        for (uint32_t i = 0; i < swapchainImages.size(); i++) {
            swapchainImageViews[i] = createImageView(swapchainImages[i], swapchainImageFormat,
                                                     VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    VkFormat
    findSupportedFormat(const std::vector<VkFormat> &candidates, const auto tiling, const auto features) {
        for (const VkFormat format: candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features ||
                (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
                return format;
            }
        }

        throw std::runtime_error("Vulkan: Failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainImageFormat;
        colorAttachment.samples = msaaSamples;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = swapchainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        std::array<VkSubpassDescription, 1> subpasses{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pColorAttachments = &colorAttachmentRef;
        subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;
        subpasses[0].pResolveAttachments = &colorAttachmentResolveRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        const std::array attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = subpasses.size();
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create render pass!");
        }
    }

    void createSwapchainFramebuffers() {
        swapchainFramebuffers.resize(swapchainImageViews.size());
        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {
                    colorImageView,
                    depthImageView,
                    swapchainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) !=
                VK_SUCCESS) {
                throw std::runtime_error("Vulkan: Failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = graphicsFamily.value();

        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create command pool!");
        }
    }

    void createCommandBuffer() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to allocate command buffers!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                throw std::runtime_error(
                        "Vulkan: Failed to create synchronization objects for frame" + std::to_string(i) + "!");
            }
        }
    }

    void createUniformDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

        const std::array bindings = {uboLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &uniformDescriptorSetLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create uniform descriptor set layout!");
        }
    }

    void createTextureDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        const std::array bindings = {samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &textureDescriptorSetLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create texture descriptor set layout!");
        }
    }

    void createUniformDescriptorPool(VkDescriptorPool *pool, const auto flags) {
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolInfo.flags = flags;

        if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, pool) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create uniform descriptor pool!");
        }
    }

    void createUniformDescriptorSets(const unsigned int bufferID, const unsigned int descriptorID, const size_t uniformSize) {
        const std::vector layouts(MAX_FRAMES_IN_FLIGHT, uniformDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = uniformDescriptorPools[bufferID];
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets[descriptorID].idRef = bufferID;
        if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[descriptorID].sets[0]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to allocate uniform descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[bufferID][i];
            bufferInfo.offset = 0;
            bufferInfo.range = uniformSize;

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[descriptorID].sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }

    unsigned int createUniformBuffer(const size_t bufferSize) {
        const unsigned int bufferID = uniformBuffers.size();
        const unsigned int descriptorID = descriptorSets.size();

        uniformBuffers.emplace_back();
        uniformBuffersMemory.emplace_back();
        uniformBuffersMapped.emplace_back();
        descriptorSets.emplace_back();
        uniformDescriptorPools.push_back({});

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[bufferID][i], uniformBuffersMemory[bufferID][i], true);
            vkmmap(&uniformBuffersMemory[bufferID][i], &uniformBuffersMapped[bufferID][i]);
        }

        createUniformDescriptorPool(&uniformDescriptorPools[bufferID], static_cast<VkDescriptorPoolCreateFlagBits>(0));
        createUniformDescriptorSets(bufferID, descriptorID, bufferSize);
        descriptorSets[descriptorID].idRef = bufferID + 1;

        return descriptorID;
    }

    void createTextureDescriptorPool(VkDescriptorPool *pool, const VkDescriptorPoolCreateFlagBits flags) {
        std::array<VkDescriptorPoolSize, 1> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1;
        poolInfo.flags = flags;

        if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, pool) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create texture descriptor pool!");
        }
    }

    void createTextureDescriptorSet(const unsigned int internalID, const VkImageView *iv, const unsigned int descriptorID) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = textureDescriptorPools[internalID];
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &textureDescriptorSetLayout;

        descriptorSets[descriptorID].idRef = 0;
        if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[descriptorID].sets[0]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to allocate texture descriptor set!");
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = *iv;
        imageInfo.sampler = textureSamplers[internalID];

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[descriptorID].sets[0];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }

    void createImage(const uint32_t width, const uint32_t height, const auto imageType, const auto format, const auto tiling,
                     const auto usage, const auto properties, VkImage &image, Allocation &alloc,
                     const unsigned int mipLevels, const auto samples) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = imageType;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

        alloc = vkalloc(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties),
                        memRequirements.alignment);

        vkBindImageMemory(logicalDevice, image, memoryBlocks[alloc.memoryRefID].memory, alloc.offset);
    }

    void createImage(const uint32_t width, const uint32_t height, const auto imageType, const auto format, const auto tiling,
                     const auto usage, const auto properties, VkImage &image, VkDeviceMemory &memory,
                     const unsigned int mipLevels, const auto samples) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = imageType;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);
        allocateDeviceMemory(memory, memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties));
        vkBindImageMemory(logicalDevice, image, memory, 0);
    }

    bool hasStencilComponent(const VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }


    void transitionImageLayout(const auto image, const auto format, const auto oldLayout, const auto newLayout,
                               const unsigned int arrayLayers, const unsigned int mipLevels) {
        const auto commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = arrayLayers;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                   newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask =
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else {
            throw std::invalid_argument("Vulkan: Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void createDepthResources() {
        const VkFormat depthFormat = findDepthFormat();
        try {
            createImage(swapchainExtent.width, swapchainExtent.height, VK_IMAGE_TYPE_2D, depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, depthImage,
                        depthImageMemory, 1, msaaSamples);
        } catch ([[maybe_unused]] std::exception &e) { // noah bug fix 2 attempt 2
            vkDestroyImage(logicalDevice, depthImage, nullptr); // noah bug fix 3 attempt 1
            createImage(swapchainExtent.width, swapchainExtent.height, VK_IMAGE_TYPE_2D, depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage,
                        depthImageMemory, 1, msaaSamples);
        }
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
    }

    void createColorResources() {
        const VkFormat colorFormat = swapchainImageFormat;
        try {
            createImage(swapchainExtent.width, swapchainExtent.height, VK_IMAGE_TYPE_2D, colorFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage,
                        colorImageMemory, 1, msaaSamples);
        } catch ([[maybe_unused]] std::exception &e) { // noah bug fix 2 attempt 2
            vkDestroyImage(logicalDevice, colorImage, nullptr); // noah bug fix 3 attempt 1
            createImage(swapchainExtent.width, swapchainExtent.height, VK_IMAGE_TYPE_2D, colorFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage,
                        colorImageMemory, 1, msaaSamples);
        }
        colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void createTextureSampler(const unsigned int id, const unsigned int mipLevels, const unsigned int samplerFilter) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = static_cast<VkFilter>(samplerFilter);
        samplerInfo.minFilter = static_cast<VkFilter>(samplerFilter);
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSamplers[id]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create texture sampler!");
        }
    }

// stupid fking minuscule premature optimization bullshit
// there is no *actual* performance difference but im hopeless
// stuff that should hardly tax the engine (2.2m triangles at 720p) absolutely murders EVERYTHING
// god im starting to hate this but not really because i love coding (more than schoolwork)
// renderdoc for mac when
    std::array<VkClearValue, 2> clearValues{};

    void setClearColor(const float r, const float g, const float b) {
        clearValues[0].color = {{r, g, b, 1.0f}};
    }

    // ReSharper disable once CppParameterNamesMismatch
    void init(GLFWwindow **window, const char *windowName, const int width, const int height, const GraphicsSettings &graphicsSettings) {
        windowPtr = window;
        settings = graphicsSettings;

        initGLFW(windowName, width, height);
        createInstance(windowName);
        createSurface();
        choosePhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();

        createCommandPool();
        createCommandBuffer();
        createSyncObjects();

        createDepthResources();
        createColorResources();
        createSwapchainFramebuffers();

        createUniformDescriptorSetLayout();
        createTextureDescriptorSetLayout();

        clearValues[0].color = {{settings.clearColor[0], settings.clearColor[1], settings.clearColor[2], 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice);
        const auto swapchainSupport = querySwapChainSupport(physicalDevice);

        createTextureDescriptorPool(&imGuiDescriptorPool, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(*windowPtr, true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = logicalDevice;
        init_info.QueueFamily = graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imGuiDescriptorPool;
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = swapchainSupport.capabilities.minImageCount;
        init_info.ImageCount = swapchainSupport.capabilities.minImageCount + 1;
        init_info.MSAASamples = msaaSamples;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info);

        const auto commandBuffer = beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        endSingleTimeCommands(commandBuffer);

        vkDeviceWaitIdle(logicalDevice);
    }

    void copyBufferToImage(const auto buffer, const auto image, const auto width, const auto height, const auto arrayLayers) {
        const auto commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = arrayLayers;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
                width,
                height,
                1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    unsigned int loadCubemap(const std::vector<std::string> &faces) {
        int texWidth[6], texHeight[6], texChannels[6];
        const unsigned int internalID = textureImages.size();
        const unsigned int descriptorID = descriptorSets.size();

        textureImages.push_back({});
        textureMemoryRefs.push_back({});
        textureImageViews.push_back({});
        textureSamplers.push_back({});
        textureMipLevels.push_back(1);
        descriptorSets.emplace_back();
        textureDescriptorPools.push_back({});

        stbi_set_flip_vertically_on_load(false);

        stbi_uc *images[6];
        for (int i = 0; i < 6; i++) {
            images[i] = stbi_load(faces[i].c_str(), &texWidth[i], &texHeight[i], &texChannels[i], STBI_rgb_alpha);
        }

        //                       width       * height       * channels
        const auto layerSize = texWidth[0] * texHeight[0] * 4;
        //  full cubemap size  = layer     * 6
        const auto imageSize = layerSize * 6;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        void *data;

        vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        for (int i = 0; i < 6; i++) {
            memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data) + layerSize * i), images[i], layerSize);
        }
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        for (auto &image: images) {
            stbi_image_free(image);
        }

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = texWidth[0];
        imageInfo.extent.height = texHeight[0];
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &textureImages[internalID]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create cubemap!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, textureImages[internalID], &memRequirements);

        textureMemoryRefs[internalID] = vkalloc(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits,
                                                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                                                memRequirements.alignment);

        vkBindImageMemory(logicalDevice, textureImages[internalID],
                          memoryBlocks[textureMemoryRefs[internalID].memoryRefID].memory,
                          textureMemoryRefs[internalID].offset);

        transitionImageLayout(textureImages[internalID], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, 1);

        copyBufferToImage(stagingBuffer, textureImages[internalID], static_cast<uint32_t>(texWidth[0]),
                          static_cast<uint32_t>(texHeight[0]), 6);

        transitionImageLayout(textureImages[internalID], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, 1);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImages[internalID];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &textureImageViews[internalID]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create cubemap image view!");
        }

        createTextureSampler(internalID, 1, VK_FILTER_LINEAR);

        createTextureDescriptorPool(&textureDescriptorPools[internalID],
                                    static_cast<VkDescriptorPoolCreateFlagBits>(0));

        createTextureDescriptorSet(internalID, &textureImageViews[internalID], descriptorID);

        return descriptorID;
    }

    void generateMipmaps(const auto image, const VkFormat imageFormat, const int32_t texWidth, const int32_t texHeight, const uint32_t mipLevels) {
        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Vulkan: Texture image format does not support linear blit!");
        }

        const auto commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    unsigned int loadTextureBytes(stbi_uc *pixels, const auto texWidth, const auto texHeight, int texChannels, const int &samplerFilter) {
        const auto internalID = textureImages.size();
        const auto descriptorID = descriptorSets.size();

        textureImages.push_back({});
        textureMemoryRefs.push_back({});
        textureImageViews.push_back({});
        textureSamplers.push_back({});
        descriptorSets.emplace_back();
        textureDescriptorPools.push_back({});

        const auto imageSize = texWidth * texHeight * 4;

        textureMipLevels.push_back(static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        void *data;

        vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImages[internalID], textureMemoryRefs[internalID],
                    textureMipLevels[internalID], VK_SAMPLE_COUNT_1_BIT);

        transitionImageLayout(textureImages[internalID], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, textureMipLevels[internalID]);

        copyBufferToImage(stagingBuffer, textureImages[internalID], static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight), 1);

        generateMipmaps(textureImages[internalID], VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight,
                        textureMipLevels[internalID]);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

        textureImageViews[internalID] = createImageView(textureImages[internalID], VK_FORMAT_R8G8B8A8_SRGB,
                                                        VK_IMAGE_ASPECT_COLOR_BIT, textureMipLevels[internalID]);

        createTextureSampler(internalID, textureMipLevels[internalID], samplerFilter);

        createTextureDescriptorPool(&textureDescriptorPools[internalID],
                                    static_cast<VkDescriptorPoolCreateFlagBits>(0));

        createTextureDescriptorSet(internalID, &textureImageViews[internalID], descriptorID);

        return descriptorID;
    }

    unsigned int loadTexture(const std::string &fileName, const int &samplerFilter) {
        stbi_set_flip_vertically_on_load(true);
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Vulkan: Failed to load image \"" + fileName + "\"!");
        }
        return loadTextureBytes(pixels, texWidth, texHeight, texChannels, samplerFilter);
    }

    unsigned int loadBundledTexture(const char *fileFirstBytePtr, const size_t fileLength, const int &samplerFilter) {
        stbi_set_flip_vertically_on_load(true);
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(fileFirstBytePtr), fileLength,
                                                &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Vulkan: Failed to load image from bundle!");
        }
        return loadTextureBytes(pixels, texWidth, texHeight, texChannels, samplerFilter);
    }

    static std::vector<char> readFile(const std::string &filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Vulkan: Failed to open file!");
        }

        const size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();

        return buffer;
    }

    inline bool ends_with(std::string const &value, std::string const &ending) {
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    unsigned int loadShader(const std::string &file_path, int target) {
        unsigned int id = shaderModuleVector.size();
        shaderModuleVector.push_back({});

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        // EDIT: These comments are stupid.
        // I fucking forgot how scope works.
        // Crucify me and never let me touch C++ again.

        // Shaders only compile if this is defined outside the if statement.
        // This was developed on an M2 MacBook, running MoltenVK 0.2.2013 (according to vulkan.gpuinfo.org)
        // I don't know why this is, but it happens.
        std::vector<unsigned int> spirv_comp;
        // For the sake of "hope it works" because I don't want to go actually manually compile SPIR-V,
        // I'm leaving this outside of the if statement too.
        std::vector<char> code;

        if (ends_with(file_path, ".spv")) {
            std::cout << "Loading " << file_path << " (compiled SPIR-V bytecode)..." << std::endl;
            code = readFile(file_path);
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
        } else {
            std::ifstream fileStream(file_path);
            if (!fileStream.good()) {
                throw std::runtime_error("Vulkan: Could not find file \"" + file_path + "\"!");
            }
            std::stringstream buffer;
            buffer << fileStream.rdbuf();
            std::string fileContents = buffer.str();
            std::cout << "Compiling " << file_path << " to SPIR-V..." << std::endl;
            bool compileSuccess = SpirvHelper::GLSLtoSPV(static_cast<VkShaderStageFlagBits>(target), &fileContents[0],
                                                         &spirv_comp);
            if (!compileSuccess) {
                throw std::runtime_error("Vulkan: Could not compile \"" + file_path + "\" to SPIR-V!");
            }
            createInfo.codeSize = spirv_comp.size() * sizeof(uint32_t);
            createInfo.pCode = reinterpret_cast<const uint32_t *>(spirv_comp.data());
        }

        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModuleVector[id]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create shader module for " + file_path + "!");
        }

        return id;
    }

    unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID,
                               const ShaderProgramSettings &shaderProgramSettings, VertexType vtype) {
        unsigned int pipelineID = pipelineLayoutVector.size();
        pipelineLayoutVector.push_back({});
        pipelineVector.push_back({});

        VkShaderModule vertex = shaderModuleVector[VertexShaderID];
        VkShaderModule fragment = shaderModuleVector[FragmentShaderID];

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertex;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragment;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = dynamicStates.size();
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = vtype == VERTEX ? InterleavedVertex::getBindingDescription()
                                                  : InterleavedAnimatedVertex::getBindingDescription();
        auto attributeDescriptions = vtype == VERTEX ? InterleavedVertex::getAttributeDescriptions()
                                                     : InterleavedAnimatedVertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchainExtent.width);
        viewport.height = static_cast<float>(swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = shaderProgramSettings.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = msaaSamples;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        if (shaderProgramSettings.transparencySupported) {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        } else {
            colorBlendAttachment.blendEnable = VK_FALSE;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPushConstantRange push_constant;
        push_constant.offset = 0;
        push_constant.size = sizeof(PushConstants);
        push_constant.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = shaderProgramSettings.testDepth;
        depthStencil.depthWriteEnable = shaderProgramSettings.transparencySupported ? VK_FALSE : VK_TRUE;
        depthStencil.depthCompareOp = shaderProgramSettings.depthAlwaysPass ? VK_COMPARE_OP_ALWAYS
                                                                            : VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};


        std::vector<VkDescriptorSetLayout> dsls = {};
        for (int i = 0; i < shaderProgramSettings.shaderInputCount; i++) {
            //  select single bit from shader inputs
            if (((shaderProgramSettings.shaderInputs >> i) & 0b1) == 1) {
                // texture
                dsls.push_back(textureDescriptorSetLayout);
            } else {
                // uniform
                dsls.push_back(uniformDescriptorSetLayout);
            }
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = dsls.size();
        pipelineLayoutInfo.pSetLayouts = dsls.data();
        pipelineLayoutInfo.pPushConstantRanges = &push_constant;
        pipelineLayoutInfo.pushConstantRangeCount = 1;

        if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayoutVector[pipelineID]) !=
            VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = pipelineLayoutVector[pipelineID];

        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                      &pipelineVector[pipelineID]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(logicalDevice, fragment, nullptr);
        vkDestroyShaderModule(logicalDevice, vertex, nullptr);

        std::cout << "Successfully created new pipeline." << std::endl;

        return pipelineID;
    }

    void cleanupSwapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(*windowPtr, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(*windowPtr, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(logicalDevice);

        vkDestroyImageView(logicalDevice, depthImageView, nullptr);
        vkDestroyImage(logicalDevice, depthImage, nullptr);
        vkFreeMemory(logicalDevice, depthImageMemory, nullptr);

        vkDestroyImageView(logicalDevice, colorImageView, nullptr);
        vkDestroyImage(logicalDevice, colorImage, nullptr);
        vkFreeMemory(logicalDevice, colorImageMemory, nullptr);

        for (const auto &swapchainFramebuffer: swapchainFramebuffers) {
            vkDestroyFramebuffer(logicalDevice, swapchainFramebuffer, nullptr);
        }

        for (const auto &swapchainImageView: swapchainImageViews) {
            vkDestroyImageView(logicalDevice, swapchainImageView, nullptr);
        }

        vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
    }

    void recreateSwapchain() {
        vkDeviceWaitIdle(logicalDevice);

        cleanupSwapchain();

        createSwapchain();
        createImageViews();
        createDepthResources();
        createColorResources();
        createSwapchainFramebuffers();
    }

    void uploadVBO(const unsigned int id, const std::vector<InterleavedVertex> *interleavedVertices, const std::vector<unsigned int> *indices) {
        VkDeviceSize bufferSize = sizeof(InterleavedVertex) * interleavedVertices->size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        void *data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, interleavedVertices->data(), bufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertexBuffers[id],
                     vertexBufferMemoryRefs[id],
                     false);

        copyBuffer(stagingBuffer, vertexBuffers[id], bufferSize);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

        // Index buffers
        bufferSize = sizeof(unsigned int) * indices->size();

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer,
                     stagingBufferMemory);

        vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices->data(), (size_t) bufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     indexBuffers[id],
                     indexBufferMemoryRefs[id],
                     false);

        copyBuffer(stagingBuffer, indexBuffers[id], bufferSize);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    unsigned int createVBO(const std::vector<InterleavedVertex> *interleavedVertices, const std::vector<unsigned int> *indices) {
        const unsigned int id = vertexBuffers.size();
        vertexBuffers.push_back({});
        vertexBufferMemoryRefs.push_back({});
        vertexBufferSizes.push_back(interleavedVertices->size() * sizeof(InterleavedVertex));
        indexBuffers.push_back({});
        indexBufferMemoryRefs.push_back({});
        indexBufferSizes.push_back(indices->size() * sizeof(unsigned int));

        uploadVBO(id, interleavedVertices, indices);

        return id;
    }

    void updateUniformBuffer(const unsigned int id, const void *ptr, const size_t size, const bool updateAll) {
        if (!updateAll) memcpy(uniformBuffersMapped[descriptorSets[id].idRef - 1][currentFrame], ptr, size);
        else
            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                memcpy(uniformBuffersMapped[descriptorSets[id].idRef - 1][i], ptr, size);
            }
    }

    VkDeviceSize offsets[] = {0};

    void renderFrame(const std::vector<Renderable *> &renderables, const std::vector<void (*)()> &imGuiCalls) {
        vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX,
                                                imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Vulkan: Failed to acquire swap chain image!");
        }

        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (const auto execute: imGuiCalls) {
            execute();
        }

        ImGui::Render();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchainExtent;
        renderPassInfo.clearValueCount = clearValues.size(); // previous constant was a horrible mistype. no more constants
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapchainExtent.height);
        viewport.width = static_cast<float>(swapchainExtent.width);
        viewport.height = -static_cast<float>(swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent;
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

        int activeProgram = -1;

        for (const auto &r: renderables) {
            if (r->shaderProgram != activeProgram) {
                activeProgram = static_cast<int>(r->shaderProgram);
                vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  pipelineVector[activeProgram]);
            }

            std::vector<VkDescriptorSet> descriptor_sets = {};
            for (const auto &d: r->descriptorIDs) {
                if (descriptorSets[d].idRef == 0) { // not uniform
                    descriptor_sets.push_back(descriptorSets[d].sets[0]);
                } else {
                    descriptor_sets.push_back(descriptorSets[d].sets[currentFrame]);
                }
            }

            vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayoutVector[activeProgram], 0, descriptor_sets.size(),
                                    descriptor_sets.data(), 0, nullptr);

            vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &vertexBuffers[r->vboID], offsets);
            vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffers[r->vboID], 0, VK_INDEX_TYPE_UINT32);

            PushConstants constants = {r->objectMatrix, r->data};
            vkCmdPushConstants(commandBuffers[currentFrame], pipelineLayoutVector[activeProgram],
                               VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants), &constants);

            vkCmdDrawIndexed(commandBuffers[currentFrame], r->indicesSize, 1, 0, 0, 0);
        }

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);

        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to record command buffer!");
        }

        constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to present swap chain image!");
        }

        currentFrame = (++currentFrame) % MAX_FRAMES_IN_FLIGHT;
    }

    void deinit() {
        cleanupSwapchain();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        vkDestroyDescriptorPool(logicalDevice, imGuiDescriptorPool, nullptr);

        for (const auto memoryBlock: memoryBlocks) {
            if (memoryBlock.mapped) vkUnmapMemory(logicalDevice, memoryBlock.memory);
            vkFreeMemory(logicalDevice, memoryBlock.memory, nullptr);
        }

        for (const auto descriptorPool: textureDescriptorPools) {
            vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
        }

        for (const auto descriptorPool: uniformDescriptorPools) {
            vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
        }

        for (const auto textureSampler: textureSamplers) {
            vkDestroySampler(logicalDevice, textureSampler, nullptr);
        }

        for (const auto textureImageView: textureImageViews) {
            vkDestroyImageView(logicalDevice, textureImageView, nullptr);
        }

        for (const auto texture: textureImages) {
            vkDestroyImage(logicalDevice, texture, nullptr);
        }

        for (const auto vertexBuffer: vertexBuffers) {
            vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
        }

        for (const auto indexBuffer: indexBuffers) {
            vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
        }

        for (const auto &ubVec: uniformBuffers) {
            for (const auto ub: ubVec) {
                vkDestroyBuffer(logicalDevice, ub, nullptr);
            }
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

        for (const auto graphicsPipelines: pipelineVector) {
            vkDestroyPipeline(logicalDevice, graphicsPipelines, nullptr);
        }

        for (const auto graphicsPipelineLayout: pipelineLayoutVector) {
            vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
        }

        vkDestroyDescriptorSetLayout(logicalDevice, uniformDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(logicalDevice, textureDescriptorSetLayout, nullptr);
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, windowSurface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(*windowPtr);
        glfwTerminate();
    }
} // JE::VK
#endif //GFX_API_VK