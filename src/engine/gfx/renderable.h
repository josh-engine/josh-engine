//
// Created by Ember Lee on 3/23/24.
//

#ifndef JOSHENGINE_RENDERABLE_H
#define JOSHENGINE_RENDERABLE_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#ifdef GFX_API_VK
#include <vulkan/vulkan.h>
#include <array>
#endif

namespace JE {
enum VertexType {
    VERTEX,
    ANIMATED_VERTEX
};

struct InterleavedVertex {
    glm::vec3 position;
    glm::vec2 uvCoords;
    glm::vec3 normal;
#ifdef GFX_API_VK
        static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(InterleavedVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.resize(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(InterleavedVertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[1].offset = offsetof(InterleavedVertex, uvCoords);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[2].offset = offsetof(InterleavedVertex, normal);

        return attributeDescriptions;
    }
#endif
};

struct InterleavedAnimatedVertex {
    glm::vec3 position;
    glm::vec2 uvCoords;
    glm::vec3 normal;
    unsigned short groupID;
#ifdef GFX_API_VK
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(InterleavedAnimatedVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.resize(4);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(InterleavedAnimatedVertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[1].offset = offsetof(InterleavedAnimatedVertex, uvCoords);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[2].offset = offsetof(InterleavedAnimatedVertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R16_UINT; // vec3
        attributeDescriptions[3].offset = offsetof(InterleavedAnimatedVertex, groupID);

        return attributeDescriptions;
    }
#endif
};

class Renderable {
public:
    unsigned int shaderProgram{};
    std::vector<unsigned int> descriptorIDs;

    glm::mat4 transform{};
    glm::mat4 rotate{};
    glm::mat4 scale{};
    glm::mat4 data{};
    glm::mat4 objectMatrix{};
    bool useLegacyNormals{};

    unsigned char flags;

    unsigned int vboID{};

    unsigned int indicesSize{};

    Renderable();

    Renderable(std::vector<float> verts, std::vector<float> _uvs, std::vector<float> norms, std::vector<unsigned int> ind, unsigned int shid, std::vector<unsigned int> descs, bool manualDepthSort);

    void setMatrices(glm::mat4 t, glm::mat4 r, glm::mat4 s);

    [[nodiscard]] bool enabled() const;
    [[nodiscard]] bool manualDepthSort() const;
    [[nodiscard]] bool checkUIBit() const;
};
}
#endif //JOSHENGINE_RENDERABLE_H