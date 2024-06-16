//
// Created by Ember Lee on 3/13/24.
//

#ifndef JOSHENGINE_MODELUTIL
#define JOSHENGINE_MODELUTIL

struct Model {
    std::vector<float> vertices{};
    std::vector<float> uvs{};
    std::vector<float> normals{};
    std::vector<unsigned int> indices{};
};

Renderable createQuad(unsigned int shader, std::vector<unsigned int> desc, bool manualDepthSort);
Renderable createQuad(unsigned int shader, std::vector<unsigned int> desc);
std::vector<Renderable> loadObj(const std::string& path, unsigned int shaderProgram, const std::vector<unsigned int>& desc, bool manualDepthSort);
std::vector<Renderable> loadObj(const std::string& path, unsigned int shaderProgram, const std::vector<unsigned int>& desc);

#endif //JOSHENGINE_MODELUTIL
