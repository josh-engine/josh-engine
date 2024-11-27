//
// Created by Ethan Lee on 7/15/24.
//

#include "uiutil.h"
#include "../../gfx/modelutil.h"
#include "../../engine.h"
#include <vector>
#include <bit> // This should fix Ubuntu

namespace JE { namespace UI {
vec2 temp_pos;
vec2 temp_size;
vec3 temp_col;
double temp_t_size;
std::string temp_str;
std::string temp_fnt;
void (*temp_fp)();
bool temp_disable;

unsigned int itemUUID = 0;
std::unordered_map<std::string, glm::vec3> fontOffsets{};

void init() {
    ShaderProgramSettings fontProgramSettings{};
    fontProgramSettings.transparencySupported = false;
    fontProgramSettings.doubleSided = true;
    fontProgramSettings.testDepth = true;
    fontProgramSettings.depthAlwaysPass = true;
    fontProgramSettings.shaderInputCount = 2;
    fontProgramSettings.shaderInputs = ShaderInputBit::Uniform | ShaderInputBit::Texture << 1;

    ShaderProgramSettings buttonProgramSettings{};
    buttonProgramSettings.transparencySupported = true;
    buttonProgramSettings.doubleSided = true;
    buttonProgramSettings.testDepth = false;
    fontProgramSettings.depthAlwaysPass = false;
    buttonProgramSettings.shaderInputCount = 1;
    buttonProgramSettings.shaderInputs = ShaderInputBit::Uniform;

    createShader("textShader", "./shaders/vertex2d_font.glsl", "./shaders/font_texture.glsl", fontProgramSettings);
    createShader("buttonShader", "./shaders/vertex2d.glsl", "./shaders/frag_button.glsl", buttonProgramSettings);
}

void createFont(const std::string& name, const std::string& atlas, glm::vec3 charScale) {
    createTexture(name, atlas);
    fontOffsets.insert({name, {charScale.x, charScale.y, charScale.z}});
}

std::vector<Renderable> stringToRenderables(std::string str, vec3 color, const std::string& font){
    std::vector<Renderable> temp{};
    int newlineCounter = 0;
    int charLineCounter = 0;
    glm::vec2 offset = fontOffsets.at(font);
    for (int i = 0; i < str.length(); i++) {
        if (str[i] != '\n') {
            temp.push_back(createQuad(getShader("textShader"), {getUBOID(), getTexture(font)}, false, true));
            temp[temp.size() - 1].data[0][0] = static_cast<float>(charLineCounter) * offset.x * 2;
            temp[temp.size() - 1].data[0][1] = static_cast<float>(newlineCounter) * -offset.y * 2;
            temp[temp.size() - 1].data[0][2] = std::bit_cast<float>(static_cast<unsigned int>(str[i]-32));
            temp[temp.size() - 1].data[1][0] = color.r;
            temp[temp.size() - 1].data[1][1] = color.g;
            temp[temp.size() - 1].data[1][2] = color.b;
            charLineCounter++;
        }
        else {
            newlineCounter++;
            charLineCounter = 0;
        }
    }
    return temp;
}

double getTextWidth(const std::string& text, const std::string& font, double size) {
    return static_cast<double>(text.length())*(fontOffsets.at(font).x*size)-(fontOffsets.at(font).z*temp_t_size/2.0);
}

void staticText(GameObject* self) {
    self->transform.position = vec3(temp_pos.x + ((fontOffsets.at(temp_fnt).x)*temp_t_size*(-static_cast<double>(temp_str.length()) + 2))+(fontOffsets.at(temp_fnt).z*temp_t_size/2.0), temp_pos.y, 0);
    self->transform.scale = vec3(static_cast<float>(temp_t_size));
    self->renderables = stringToRenderables(temp_str, temp_col, temp_fnt);
}

bool lastButtonDown = false;
void buttonUpdate(double _, GameObject* self) {
    vec2 mouse = getCursorPos();
    if (mouse.x > self->transform.position.x-self->transform.scale.x
     && mouse.x < self->transform.position.x+self->transform.scale.x
     && mouse.y > self->transform.position.y-self->transform.scale.y
     && mouse.y < self->transform.position.y+self->transform.scale.y) {
        self->renderables[0].data[0][3] = 0.4f;
        bool buttonDown = isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        if (!buttonDown && lastButtonDown) {
            (*std::bit_cast<void (*)()>(self->flags))(); // Call pointer stored in flags
        }
        lastButtonDown = buttonDown;
    } else {
        self->renderables[0].data[0][3] = 0.6f;
    }
}

void clickableButton(GameObject* self) {
    self->transform.position = vec3(temp_pos, -1);
    self->transform.scale = vec3(temp_size, 1);
    self->renderables.push_back(createQuad(getShader("buttonShader"), {getUBOID()}, false, true));
    self->renderables[0].data[0][0] = 0.0f;
    self->renderables[0].data[0][1] = 0.0f;
    self->renderables[0].data[0][2] = 0.0f;
    self->renderables[0].data[0][3] = 0.6f;
    self->flags = std::bit_cast<uint64>(temp_fp);
    if (!temp_disable) self->onUpdate.push_back(&buttonUpdate);
}

void staticText(const glm::vec2& pos, const std::string& text, const std::string& font, const double& textSize, const glm::vec3& color) {
    temp_pos = pos;
    temp_str = text;
    temp_col = color;
    temp_t_size = textSize;
    temp_fnt = font;
    putGameObject("uiTextObj_"+text+"_"+std::to_string(itemUUID++), GameObject(&staticText));
}

void staticButton(const glm::vec2& pos, const std::string& text, const std::string& font, void (*click_function)(), const glm::vec2& padding, const double& textSize, const glm::vec3& color, bool disabled) {
    staticText(pos, text, font, textSize, color * (disabled ? vec3(0.3) : vec3(1)));
    temp_size = {getTextWidth(text, font, textSize)+padding.x, fontOffsets.at(font).y*textSize+padding.y};
    temp_fp = click_function;
    temp_disable = disabled;
    putGameObject("uiButtonObj_"+text+"_"+std::to_string(itemUUID++), GameObject(&clickableButton));
}
}}