//
// Created by Ember Lee on 3/23/24.
//

#ifndef JOSHENGINE_RENDERABLE_H
#define JOSHENGINE_RENDERABLE_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#ifdef GFX_API_OPENGL41
#include "GLFW/glfw3.h"
#endif

#ifdef GFX_API_VK
#include <vulkan/vulkan.h>
#include <array>

struct JEInterleavedVertex_VK {
    glm::vec3 position;
    glm::vec2 uvCoords;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(JEInterleavedVertex_VK);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(JEInterleavedVertex_VK, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[1].offset = offsetof(JEInterleavedVertex_VK, uvCoords);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[2].offset = offsetof(JEInterleavedVertex_VK, normal);

        return attributeDescriptions;
    }
};

unsigned int createVBOFunctionMirror(void* r);
#endif

#ifdef GFX_API_MTL
struct JEInterleavedVertex_MTL {
    alignas(16) glm::vec3 position;
    alignas(8 ) glm::vec2 textureCoordinate;
    alignas(16) glm::vec3 normal;
};

unsigned int createVBOFunctionMirror(void* r);
#endif

class Renderable {
public:
    std::vector<float> vertices;
    std::vector<float> uvs;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

    unsigned int shaderProgram{};
    unsigned int texture{};

    glm::mat4 transform{};
    glm::mat4 rotate{};
    glm::mat4 scale{};
    glm::mat4 objectMatrix{};

    bool enabled;

    bool manualDepthSort{};

#ifdef GFX_API_OPENGL41
    unsigned int vboID; // Positions
    unsigned int tboID; // UV coordinates
    unsigned int nboID; // Normals
    unsigned int iboID; // Indices
#endif //GFX_API_OPENGL41

#ifdef GFX_API_VK
    unsigned int vboID{};
    std::vector<JEInterleavedVertex_VK> interleavedVertices;
#endif

#ifdef GFX_API_MTL
    unsigned int vboID{};
    std::vector<JEInterleavedVertex_MTL> interleavedVertices;
#endif

    // Although on OpenGL this was negligible, for some reason on Vulkan this resulted in around +10FPS.
    unsigned int indicesSize{};

    Renderable() {
        enabled = false;
    }

    Renderable(std::vector<float> verts, std::vector<float> _uvs, std::vector<float> norms, std::vector<unsigned int> ind, unsigned int shid, unsigned int tex, bool manualDepthSort) {
        enabled = true;
        vertices = std::move(verts);
        uvs = std::move(_uvs);
        normals = std::move(norms);
        indices = std::move(ind);
        shaderProgram = shid; // Once in a blue moon do you realize what you just named a variable... I'm keeping it.
        texture = tex;
        this->manualDepthSort = manualDepthSort;

#ifdef GFX_API_OPENGL41
        // Vertex Buffer
        glGenBuffers(1, &vboID); // reserve an ID for our VBO
        glBindBuffer(GL_ARRAY_BUFFER, vboID); // bind VBO
        glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(float),
                &vertices[0],
                GL_STATIC_DRAW
        );


        // Texture Coordinate Buffer
        glGenBuffers(1, &tboID);
        glBindBuffer(GL_ARRAY_BUFFER, tboID);
        glBufferData(
                GL_ARRAY_BUFFER,
                uvs.size() * sizeof(float),
                &uvs[0],
                GL_STATIC_DRAW
        );

        // Normals Buffer
        glGenBuffers(1, &nboID);
        glBindBuffer(GL_ARRAY_BUFFER, nboID);
        glBufferData(
                GL_ARRAY_BUFFER,
                normals.size() * sizeof(float),
                &normals[0],
                GL_STATIC_DRAW
        );

        // Indices Buffer
        glGenBuffers(1, &iboID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
        glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(unsigned int),
                &indices[0],
                GL_STATIC_DRAW
        );
#endif //GFX_API_OPENGL41
#if defined(GFX_API_VK) | defined(GFX_API_MTL)
        for (int i = 0; i < vertices.size()/3; i++) {
            interleavedVertices.push_back({
                                                  {vertices[3*i],  vertices[(3*i)+1], vertices[(3*i)+2]},
                                                  {uvs[(2*i)],     uvs[(2*i)+1]},
                                                  {normals[(3*i)], normals[(3*i)+1],  normals[(3*i)+2]}
            });
        }

        vboID = createVBOFunctionMirror(this);
#endif
        indicesSize = indices.size();
    }

    void setMatrices(glm::mat4 t, glm::mat4 r, glm::mat4 s) {
        this->transform = t;
        this->rotate = r;
        this->scale = s;
        this->objectMatrix = (this->transform * this->rotate * this->scale);
    }

    void saveToFile(const std::string& fileName);
};

Renderable renderableFromFile(const std::string& fileName);

#endif //JOSHENGINE_RENDERABLE_H