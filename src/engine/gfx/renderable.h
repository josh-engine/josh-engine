//
// Created by Ember Lee on 3/23/24.
//

#ifndef JOSHENGINE_RENDERABLE_H
#define JOSHENGINE_RENDERABLE_H

#include "../engineconfig.h"
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <vector>

#ifdef GFX_API_OPENGL41
#include "GLFW/glfw3.h"
#endif

#ifdef GFX_API_VK
#include <vulkan/vulkan.h>
#include <array>

struct JEInterleavedVertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uvCoords;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(JEInterleavedVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[0].offset = offsetof(JEInterleavedVertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[1].offset = offsetof(JEInterleavedVertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attributeDescriptions[2].offset = offsetof(JEInterleavedVertex, uvCoords);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attributeDescriptions[3].offset = offsetof(JEInterleavedVertex, normal);

        return attributeDescriptions;
    }
};
#endif

#ifdef GFX_API_VK
unsigned int createVBOFunctionMirror(void* r);
#endif

class Renderable {
public:
    std::vector<float> vertices;
    std::vector<float> colors;
    std::vector<float> uvs;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

    unsigned int shaderProgram;
    unsigned int texture;

    glm::mat4 transform;
    glm::mat4 rotate;
    glm::mat4 scale;

    bool enabled;

#ifdef GFX_API_OPENGL41
    unsigned int vboID; // Positions
    unsigned int cboID; // Colors
    unsigned int tboID; // UV coordinates
    unsigned int nboID; // Normals
    unsigned int iboID; // Indices
#endif //GFX_API_OPENGL41

#ifdef GFX_API_VK
    unsigned int vboID;
    std::vector<JEInterleavedVertex> interleavedVertices;
#endif

    glm::mat4 objectMatrix(){
        return (transform * rotate * scale);
    }

    Renderable(){
        enabled = false;
    }

    Renderable(std::vector<float> verts, std::vector<float> cols, std::vector<float> _uvs, std::vector<float> norms, std::vector<unsigned int> ind, unsigned int shid, unsigned int tex){
        enabled = true;
        vertices = std::move(verts);
        colors = std::move(cols);
        uvs = std::move(_uvs);
        normals = std::move(norms);
        indices = std::move(ind);
        shaderProgram = shid; // Once in a blue moon do you realize what you just named a variable... I'm keeping it.
        texture = tex;

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

        // Color Buffer
        glGenBuffers(1, &cboID);
        glBindBuffer(GL_ARRAY_BUFFER, cboID);
        glBufferData(
                GL_ARRAY_BUFFER,
                colors.size() * sizeof(float),
                &colors[0],
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
#ifdef GFX_API_VK
        for (int i = 0; i < vertices.size()/3; i++){
            interleavedVertices.push_back({
                                                  {vertices[3*i],  vertices[(3*i)+1], vertices[(3*i)+2]},
                                                  {colors[(3*i)],  colors[(3*i)+1],   colors[(3*i)+2]},
                                                  {uvs[(2*i)],     uvs[(2*i)+1]},
                                                  {normals[(3*i)], normals[(3*i)+1],  normals[(3*i)+2]}});
        }

        vboID = createVBOFunctionMirror(this);
#endif //GFX_API_VK
    }

    void setMatrices(glm::mat4 transform, glm::mat4 rotation, glm::mat4 scale){
        this->transform = transform;
        this->rotate = rotation;
        this->scale = scale;
    }
};

#endif //JOSHENGINE_RENDERABLE_H