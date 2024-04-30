//
// Created by Ember Lee on 3/9/24.
//

#ifndef JOSHENGINE_ENGINE_H
#define JOSHENGINE_ENGINE_H
#if defined(GFX_API_VK) | defined(GFX_API_MTL)
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif
#include <glm/glm.hpp>
#include <map>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "gfx/renderable.h"

using namespace glm;

struct JEGraphicsSettings {
    bool vsyncEnabled;
    bool skybox;
    float clearColor[3];
    int msaaSamples;
};

class Transform {
public:
    vec3 position{};
    vec3 rotation{};
    vec3 scale{};

    Transform() {
        this->position = vec3(0.0f);
        this->rotation = vec3(0.0f);
        this->scale = vec3(1.0f);
    }

    explicit Transform(vec3 pos) {
        this->position = pos;
        this->rotation = vec3(0.0f);
        this->scale = vec3(1.0f);
    }

    Transform(vec3 pos, vec3 rot) {
        this->position = pos;
        this->rotation = rot;
        this->scale = vec3(1.0f);
    }

    Transform(vec3 pos, vec3 rot, vec3 sca) {
        this->position = pos;
        this->rotation = rot;
        this->scale = sca;
    }

    [[nodiscard]] vec3 direction() const {
        return {cos(radians(this->rotation.y)) * sin(radians(this->rotation.x)),
                sin(radians(this->rotation.y)),
                cos(radians(this->rotation.y)) * cos(radians(this->rotation.x))};
    }

    [[nodiscard]] mat4 getTranslateMatrix() const {
        return translate(mat4(1.0f), position);
    }

    [[nodiscard]] mat4 getRotateMatrix() const {
        vec3 radianRotation = radians(rotation);
        auto rotationMatrix = identity<mat4>();
        //TODO: There HAS to be a better way to do this.
        rotationMatrix = rotate(rotationMatrix, radianRotation.x, vec3(1, 0, 0));
        rotationMatrix = rotate(rotationMatrix, radianRotation.y, vec3(0, 1, 0));
        rotationMatrix = rotate(rotationMatrix, radianRotation.z, vec3(0, 0, 1));
        return rotationMatrix;
    }

    [[nodiscard]] mat4 getScaleMatrix() const {
        return glm::scale(mat4(1.0f), scale);
    }
};

class GameObject {
public:
    Transform transform;
    std::vector<void (*)(double dt, GameObject* g)> onUpdate = {};
    std::vector<Renderable> renderables = {};

    explicit GameObject(void (*initFunc)(GameObject *g)) {
        transform = Transform();
        initFunc(this);
    }

    GameObject(Transform t, void (*initFunc)(GameObject *g)) {
        transform = t;
        initFunc(this);
    }
};

void init(const char* windowName, int width, int height, JEGraphicsSettings settings);
void mainLoop();
void deinit();
void registerOnUpdate(void (*function)(double dt));
void registerOnKey(void (*function)(int key, bool pressed, double dt));
void putGameObject(std::string name, GameObject g);
std::unordered_map<std::string, GameObject*> getGameObjects();
void putImGuiCall(void (*argument)());
GameObject* getGameObject(std::string name);
void setFOV(float n);
int getCurrentWidth();
int getCurrentHeight();
Transform* cameraAccess();
bool isKeyDown(int key);
bool isMouseButtonDown(int button);
GLFWwindow** getWindow();
vec2 getRawCursorPos();
vec2 getCursorPos();
void setRawCursorPos(vec2 pos);
void setCursorPos(vec2 pos);
unsigned int createTextureWithName(std::string name, std::string fileName);
unsigned int createTexture(std::string folderPath, std::string fileName);
unsigned int getTexture(const std::string& name);
bool textureExists(const std::string &name);
unsigned int getProgram(const std::string& name);
void registerProgram(std::string name, std::string vertex, std::string fragment, bool testDepth, bool transparencySupported, bool doubleSided);
double getFrameTime();
int getRenderableCount();
void setMouseVisible(bool vis);
void setSunProperties(vec3 position, vec3 color);
void setAmbient(float r, float g, float b);
void setAmbient(vec3 rgb);
void registerOnMouse(void (*function)(int button, bool pressed, double dt));
void setSkyboxEnabled(bool enabled);
std::string textureReverseLookup(unsigned int num);
std::string programReverseLookup(unsigned int num);
#ifdef DEBUG_ENABLED
std::unordered_map<std::string, unsigned int> getTexs();
#endif

#endif //JOSHENGINE_ENGINE_H
