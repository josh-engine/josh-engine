//
// Created by Ember Lee on 3/13/24.
//

#ifndef JOSHENGINE_MODELUTIL
#define JOSHENGINE_MODELUTIL

Renderable createQuad(bool is3d, GLuint shader, GLuint texture);
std::vector<Renderable> loadObj(std::string path, unsigned int shaderProgram);

#endif //JOSHENGINE_MODELUTIL
