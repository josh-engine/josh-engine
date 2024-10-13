//
// Created by Ethan Lee on 7/15/24.
//

#ifndef JOSHENGINE_UIUTIL_H
#define JOSHENGINE_UIUTIL_H

#include "../../gfx/renderable.h"
#include <string>

namespace JE {
/**
 * I don't know, what do you think it does? Initialize the audio? /s
 */
void initUI();

/**
 * Create a font atlas texture in memory, with optional (shit will look weird without this) scaling parameters.
 * @param name The font texture's name.
 * @param atlas The directory of the font atlas.
 * @param charScale {Horizontal Square-Relative Width, Vertical Square-Relative Width, Square-Relative Right Spacing}
*/
void createFont(const std::string& name, const std::string& atlas, glm::vec3 charScale = {1.0f, 1.0f, 0.0f});

/**
 * Creates a list of renderables from text that can be put in a GameObject to display on-screen.
 * @param str Text string
 * @param color Text color
 * @param font Text font (name passed to createFont)
 * @return A vector of Renderables, one quad renderable per character.
 */
std::vector<Renderable> stringToRenderables(std::string str, glm::vec3 color, const std::string& font);

// TODO: add docs
void uiStaticText(const glm::vec2& pos, const std::string& text, const std::string& fontTex, const double& textSize = 0.25, const glm::vec3& color = {1.0, 1.0, 1.0});
// TODO: add docs
void uiStaticButton(const glm::vec2& pos, const std::string& text, const std::string& font, void (*click_function)(), const glm::vec2& padding = {0.05, 0.05}, const double& textSize = 0.25, const glm::vec3& color = {1.0, 1.0, 1.0}, bool disabled = false);
}
#endif //JOSHENGINE_UIUTIL_H