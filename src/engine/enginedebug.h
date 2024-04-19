//
// Created by Ember Lee on 3/24/24.
//

#ifndef JOSHENGINE_ENGINEDEBUG_H
#define JOSHENGINE_ENGINEDEBUG_H

#include <string>

void initDebugTools();
void setupImGuiWindow();
void registerFunctionToDebug(const std::string& name, void* function);

#endif //JOSHENGINE_ENGINEDEBUG_H
