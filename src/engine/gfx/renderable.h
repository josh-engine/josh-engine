//
// Created by Ember Lee on 3/23/24.
//

#ifndef JOSHENGINE_RENDERABLE_H
#define JOSHENGINE_RENDERABLE_H

#include "../engineconfig.h"

#ifdef GFX_API_OPENGL33
#include "GLFW/glfw3.h"
#endif

#include <glm/glm.hpp>
#include <vector>

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
    bool is3d;
    bool testDepth;

#ifdef GFX_API_OPENGL33
    unsigned int vboID; // Positions
    unsigned int cboID; // Colors
    unsigned int tboID; // UV coordinates
    unsigned int nboID; // Normals
    unsigned int iboID; // Indices
#endif //GFX_API_OPENGL33

    glm::mat4 objectMatrix(){
        return (transform * rotate * scale);
    }

    Renderable(){
        enabled = false;
    }

    Renderable(bool _3d, std::vector<float> verts, std::vector<float> cols, std::vector<float> _uvs, std::vector<float> norms, std::vector<unsigned int> ind, unsigned int shid, unsigned int tex, bool testDepth){
        enabled = true;
        is3d = _3d;
        vertices = std::move(verts);
        colors = std::move(cols);
        uvs = std::move(_uvs);
        normals = std::move(norms);
        indices = std::move(ind);
        shaderProgram = shid; // Once in a blue moon do you realize what you just named a variable... I'm keeping it.
        texture = tex;
        this->testDepth = testDepth;

#ifdef GFX_API_OPENGL33
        // Vertex Buffer
        glGenBuffers(1, &vboID); // reserve an ID for our VBO
        glBindBuffer(GL_ARRAY_BUFFER, vboID); // bind VBO
        glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(GLfloat),
                &vertices[0],
                GL_STATIC_DRAW
        );

        // Color Buffer
        glGenBuffers(1, &cboID);
        glBindBuffer(GL_ARRAY_BUFFER, cboID);
        glBufferData(
                GL_ARRAY_BUFFER,
                colors.size() * sizeof(GLfloat),
                &colors[0],
                GL_STATIC_DRAW
        );

        // Texture Coordinate Buffer
        glGenBuffers(1, &tboID);
        glBindBuffer(GL_ARRAY_BUFFER, tboID);
        glBufferData(
                GL_ARRAY_BUFFER,
                uvs.size() * sizeof(GLfloat),
                &uvs[0],
                GL_STATIC_DRAW
        );

        // Normals Buffer
        glGenBuffers(1, &nboID);
        glBindBuffer(GL_ARRAY_BUFFER, nboID);
        glBufferData(
                GL_ARRAY_BUFFER,
                normals.size() * sizeof(GLfloat),
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
#endif //GFX_API_OPENGL33
    }

    void setMatrices(glm::mat4 transform, glm::mat4 rotation, glm::mat4 scale){
        this->transform = transform;
        this->rotate = rotation;
        this->scale = scale;
    }
};

#endif //JOSHENGINE_RENDERABLE_H
