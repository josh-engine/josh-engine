//
// Created by Ember Lee on 3/27/24.
//
#include <fstream>
#include <istream>
#include <filesystem>
#include <iostream>
#include <iterator>
#include "renderable.h"
#include "../engine.h"
#include <bit>

#ifdef GFX_API_VK
#include "vk/gfx_vk.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO(reinterpret_cast<Renderable*>(r));
}
#endif

#ifdef GFX_API_MTL
#include "mtl/gfx_mtl.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO(reinterpret_cast<Renderable*>(r));
}
#endif

Renderable::Renderable(std::vector<float> verts, std::vector<float> _uvs, std::vector<float> norms, std::vector<unsigned int> ind, unsigned int shid, std::vector<unsigned int> descs, bool manualDepthSort) {
    enabled = true;
    vertices = std::move(verts);
    uvs = std::move(_uvs);
    normals = std::move(norms);
    indices = std::move(ind);
    descriptorIDs= std::move(descs);
    this->shaderProgram = shid;
    this->manualDepthSort = manualDepthSort;

    for (int i = 0; i < vertices.size()/3; i++) {
        interleavedVertices.push_back({
            {vertices[3*i],  vertices[(3*i)+1], vertices[(3*i)+2]},
            {uvs[(2*i)],     uvs[(2*i)+1]},
            {normals[(3*i)], normals[(3*i)+1],  normals[(3*i)+2]}
        });
    }

    vboID = createVBOFunctionMirror(this);
    indicesSize = indices.size();
}