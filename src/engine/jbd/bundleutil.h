//
// Created by Ethan Lee on 8/19/24.
//

#ifndef JOSHENGINE_BUNDLEUTIL_H
#define JOSHENGINE_BUNDLEUTIL_H

#include <vector>
#include <string>

namespace JE {
std::vector<unsigned char> getFileCharVec(const std::string& extractFileName, const std::string& bundleFileName);
}
#endif //JOSHENGINE_BUNDLEUTIL_H
