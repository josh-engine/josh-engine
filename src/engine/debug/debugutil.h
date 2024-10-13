//
// Created by Ember Lee on 3/24/24.
//

#ifndef JOSHENGINE_DEBUGUTIL_H
#define JOSHENGINE_DEBUGUTIL_H

#include <string>

namespace JE {
void initDebugTools();
void setupImGuiWindow();
void registerFunctionToDebug(const std::string& name, void* function);
}
#endif //JOSHENGINE_DEBUGUTIL_H
