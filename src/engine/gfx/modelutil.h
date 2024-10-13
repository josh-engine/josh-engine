//
// Created by Ember Lee on 3/13/24.
//

#ifndef JOSHENGINE_MODELUTIL
#define JOSHENGINE_MODELUTIL

namespace JE {
struct Model {
    std::vector<float> vertices{};
    std::vector<float> uvs{};
    std::vector<float> normals{};
    std::vector<unsigned int> indices{};
};

Renderable createQuad(unsigned int shader, std::vector<unsigned int> desc, bool manualDepthSort = false, bool ui = false);
std::vector<Renderable> loadObj(const std::string& path, unsigned int shaderProgram, const std::vector<unsigned int>& desc, bool manualDepthSort = false);
std::vector<Renderable> loadObj(const std::string& path, const std::string& bundleFileName,  unsigned int shaderProgram, const std::vector<unsigned int>& desc, bool manualDepthSort = false);
}
#endif //JOSHENGINE_MODELUTIL