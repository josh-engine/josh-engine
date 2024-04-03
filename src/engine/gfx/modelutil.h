//
// Created by Ember Lee on 3/13/24.
//

#ifndef JOSHENGINE_MODELUTIL
#define JOSHENGINE_MODELUTIL

Renderable createQuad(unsigned int shader, unsigned int texture, bool manualDepthSort);
Renderable createQuad(unsigned int shader, unsigned int texture);
std::vector<Renderable> loadObj(std::string path, unsigned int shaderProgram, bool manualDepthSort);
std::vector<Renderable> loadObj(std::string path, unsigned int shaderProgram);

#endif //JOSHENGINE_MODELUTIL
