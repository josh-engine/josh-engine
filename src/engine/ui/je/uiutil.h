//
// Created by Ethan Lee on 7/15/24.
//

#ifndef JOSHENGINE_UIUTIL_H
#define JOSHENGINE_UIUTIL_H

#include "../../gfx/renderable.h"
#include <string>

void initUI();
std::vector<Renderable> stringToRenderables(std::string str, glm::vec3 color);
void uiStaticText(const glm::vec2& pos, const std::string& text, const float& textSize = 0.025f, const glm::vec3& color = {1.0, 1.0, 1.0});
void uiButton(const glm::vec2& pos, const std::string& text, void (*click_function)(), const glm::vec2& padding = {0.005, 0.005}, const float& textSize = 0.025f, const glm::vec3& color = {1.0, 1.0, 1.0}, bool disabled = false);

#endif //JOSHENGINE_UIUTIL_H
