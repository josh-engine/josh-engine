//
// Created by Ember Lee on 3/27/24.
//
#include <istream>
#include <iterator>
#include "renderable.h"

#ifdef GFX_API_VK
#include "vk/gfx_vk.h"

unsigned int createVBOFunctionMirror(void* r, void* v, void* i) {
    return createVBO(reinterpret_cast<std::vector<JEInterleavedVertex_VK> *>(v),
                     reinterpret_cast<std::vector<unsigned int> *>(i));
}
#endif

Renderable::Renderable(std::vector<float> vertices, std::vector<float> uvs, std::vector<float> normals, std::vector<unsigned int> indices, unsigned int shid, std::vector<unsigned int> descs, bool manualDepthSort) {
    flags = 0b1;
    descriptorIDs= std::move(descs);
    this->shaderProgram = shid;
    this->manualDepthSort = manualDepthSort;

    std::vector<JEInterleavedVertex_VK> interleavedVertices{};
    interleavedVertices.reserve(vertices.size()/3);
    for (int i = 0; i < vertices.size()/3; i++) {
        interleavedVertices.push_back({
            {vertices[3*i],  vertices[(3*i)+1], vertices[(3*i)+2]},
            {uvs[(2*i)],     uvs[(2*i)+1]},
            {normals[(3*i)], normals[(3*i)+1],  normals[(3*i)+2]}
        });
    }

    vboID = createVBOFunctionMirror(this, &interleavedVertices, &indices);
    indicesSize = indices.size();
}

Renderable::Renderable() {
    flags = 0;
}

void Renderable::setMatrices(glm::mat4 t, glm::mat4 r, glm::mat4 s) {
    this->transform = t;
    this->rotate = r;
    this->scale = s;
    this->objectMatrix = (this->transform * this->rotate * this->scale);
}

bool Renderable::enabled() const {
    return (this->flags & 0b1) == 1;
}