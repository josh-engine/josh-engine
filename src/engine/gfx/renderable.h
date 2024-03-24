//
// Created by Ethan Lee on 3/23/24.
//

#ifndef JOSHENGINE_RENDERABLE_H
#define JOSHENGINE_RENDERABLE_H
#include <glm/glm.hpp>
#include <vector>

class Renderable {
public:
    bool enabled;
    bool is3d;
    bool testDepth;
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
    }

    Renderable addMatrices(glm::mat4 transform, glm::mat4 rotation, glm::mat4 scale){
        Renderable r = Renderable(this->is3d, this->vertices, this->colors, this->uvs, this->normals, this->indices, this->shaderProgram, this->texture, this->testDepth);
        r.transform = transform;
        r.rotate = rotation;
        r.scale = scale;
        return r;
    }
};

#endif //JOSHENGINE_RENDERABLE_H
