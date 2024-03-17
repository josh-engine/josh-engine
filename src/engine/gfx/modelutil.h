//
// Created by Ethan Lee on 3/13/24.
//

#ifndef JOSHENGINE3_1_2DUTIL_H
#define JOSHENGINE3_1_2DUTIL_H

#include "enginegfx.h"

Renderable createQuad(bool is3d, GLuint shader, GLuint texture){
    return Renderable(
            is3d, //3d?
            {-1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f},    //verts
            { 1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f},   //colors
            { 0.0f,  0.0f,          1.0f,  0.0f,           0.0f,  1.0f       ,  1.0f,  1.0f},  //UVs
            { 0.0f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,    0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f}, //normals
            { 0, 1, 2,     1, 3, 2 }, //indices
            shader,    //shader program
            texture    //texture
    );
}

#endif //JOSHENGINE3_1_2DUTIL_H
