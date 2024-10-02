//
// Created by Ethan Lee on 7/15/24.
//

#include "uiutil.h"
#include "../../gfx/modelutil.h"
#include "../../engine.h"
#include <vector>

vec2 temp_pos;
vec2 temp_size;
vec3 temp_col;
float temp_t_size;
std::string temp_str;
std::string temp_fnt;
void (*temp_fp)();
bool temp_disable;

unsigned int itemUUID = 0;

void initUI() {
    JEShaderProgramSettings fontProgramSettings{};
    fontProgramSettings.transparencySupported = false; // Just discard; empty pixels because pixel font
    fontProgramSettings.doubleSided = true;
    fontProgramSettings.testDepth = false;
    fontProgramSettings.depthAlwaysPass = true;
    fontProgramSettings.shaderInputCount = 2;
    fontProgramSettings.shaderInputs = JEShaderInputUniformBit | JEShaderInputTextureBit << 1;

    JEShaderProgramSettings buttonProgramSettings{};
    buttonProgramSettings.transparencySupported = true;
    buttonProgramSettings.doubleSided = true;
    buttonProgramSettings.testDepth = false;
    buttonProgramSettings.shaderInputCount = 1;
    buttonProgramSettings.shaderInputs = JEShaderInputUniformBit;

    createShader("textShader", "./shaders/vertex2d_font.glsl", "./shaders/font_texture.glsl", fontProgramSettings);
    createShader("buttonShader", "./shaders/vertex2d.glsl", "./shaders/frag_button.glsl", buttonProgramSettings);
}

std::vector<Renderable> stringToRenderables(std::string str, vec3 color, std::string font){
    std::vector<Renderable> temp{};
    int newlineCounter = 0;
    int charLineCounter = 0;
    for (int i = 0; i < str.length(); i++) {
        if (str[i] != '\n') {
            temp.push_back(createQuad(getShader("textShader"), {getUBOID(), getTexture(font)}, true));
            temp[temp.size() - 1].useFakedNormalMatrix = true;
            temp[temp.size() - 1].normal[0][0] = static_cast<float>(charLineCounter * 2);
            temp[temp.size() - 1].normal[0][1] = static_cast<float>(newlineCounter * -2);
            temp[temp.size() - 1].normal[0][2] = std::bit_cast<float>(static_cast<unsigned int>(str[i]));
            temp[temp.size() - 1].normal[1][0] = color.r;
            temp[temp.size() - 1].normal[1][1] = color.g;
            temp[temp.size() - 1].normal[1][2] = color.b;
            charLineCounter++;
        }
        else {
            newlineCounter++;
            charLineCounter = 0;
        }
    }
    return temp;
}

void staticText(GameObject* self) {
    self->transform.position = vec3(temp_pos.x-(((temp_str.length()-1)/2.0f)*temp_t_size*2), temp_pos.y, 0);
    self->transform.scale = vec3(temp_t_size);
    self->renderables = stringToRenderables(temp_str, temp_col, temp_fnt);
}

bool lastButtonDown = false;
void buttonUpdate(double dt, GameObject* self) {
    vec2 mouse = getCursorPos();
    if (mouse.x > self->transform.position.x-self->transform.scale.x
     && mouse.x < self->transform.position.x+self->transform.scale.x
     && mouse.y > self->transform.position.y-self->transform.scale.y
     && mouse.y < self->transform.position.y+self->transform.scale.y) {
        self->renderables[0].normal[0][3] = 0.4f;
        bool buttonDown = isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        if (!buttonDown && lastButtonDown) {
            (*std::bit_cast<void (*)()>(self->flags))(); // Call pointer stored in flags
        }
        lastButtonDown = buttonDown;
    } else {
        self->renderables[0].normal[0][3] = 0.6f;
    }
}

void clickableButton(GameObject* self) {
    self->transform.position = vec3(temp_pos, -1);
    self->transform.scale = vec3(temp_size, 1);
    self->renderables.push_back(createQuad(getShader("buttonShader"), {getUBOID()}, true));
    self->renderables[0].useFakedNormalMatrix = true;
    self->renderables[0].normal[0][0] = 0.0f;
    self->renderables[0].normal[0][1] = 0.0f;
    self->renderables[0].normal[0][2] = 0.0f;
    self->renderables[0].normal[0][3] = 0.6f;
    self->flags = std::bit_cast<uint64>(temp_fp);
    if (!temp_disable) self->onUpdate.push_back(&buttonUpdate);
}

void uiStaticText(const glm::vec2& pos, const std::string& text, const std::string& font, const float& textSize, const glm::vec3& color) {
    temp_pos = pos;
    temp_str = text;
    temp_col = color;
    temp_t_size = textSize;
    temp_fnt = font;
    putGameObject("uiTextObj_"+text+"_"+std::to_string(itemUUID++), GameObject(&staticText));
}

void uiStaticButton(const glm::vec2& pos, const std::string& text, const std::string& font, void (*click_function)(), const glm::vec2& padding, const float& textSize, const glm::vec3& color, bool disabled) {
    uiStaticText(pos, text, font, textSize, color * (disabled ? vec3(0.3) : vec3(1)));
    temp_size = {textSize*text.length()+padding.x, textSize+padding.y};
    temp_fp = click_function;
    temp_disable = disabled;
    putGameObject("uiButtonObj_"+text+"_"+std::to_string(itemUUID++), GameObject(&clickableButton));
}