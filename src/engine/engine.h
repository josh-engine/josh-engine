//
// Created by Ember Lee on 3/9/24.
//

#ifndef JOSHENGINE_ENGINE_H
#define JOSHENGINE_ENGINE_H
#if defined(GFX_API_VK)
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#endif
#include <glm/glm.hpp>
#include <map>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "gfx/renderable.h"
#include <unordered_map>

using namespace glm;
typedef glm::vec<2, float, (qualifier)3> vec2_MSVC;
typedef glm::vec<3, float, (qualifier)3> vec3_MSVC;

#define JEShaderInputUniformBit 0
#define JEShaderInputTextureBit 1

enum JETextureFilter {
    JE_PIXEL_ART = 0,
    JE_TEXTURE = 1
};

struct JEGraphicsSettings {
    bool  vsyncEnabled;
    bool  skybox;
    float clearColor[3];
    int   msaaSamples;
};

struct JEShaderProgramSettings {
    bool testDepth;
    bool transparencySupported;
    bool doubleSided;
    bool depthAlwaysPass = false;
    u32  shaderInputs;
    u8   shaderInputCount;
};

class Transform {
public:
    vec3 position{};
    vec3 rotation{};
    vec3 scale{};

    vec3 pos_vel = vec3(0);
    vec3 rot_vel = vec3(0);

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

    [[nodiscard]] mat4 getRotateMatrix() const;

    [[nodiscard]] mat4 getScaleMatrix() const {
        return glm::scale(mat4(1.0f), scale);
    }
};

class GameObject {
public:
    Transform transform;
    std::vector<void (*)(double dt, GameObject* g)> onUpdate = {};
    std::vector<Renderable> renderables = {};
    union { //TODO maybe more things
        uint64_t flags = 0;
    };

    explicit GameObject(void (*initFunc)(GameObject *g)) {
        transform = Transform();
        initFunc(this);
    }

    GameObject(Transform t, void (*initFunc)(GameObject *g)) {
        transform = t;
        initFunc(this);
    }
};

struct JEUniformBufferObject {
    alignas(16) mat4 view;
    alignas(16) mat4 _2dProj;
    alignas(16) mat4 _3dProj;
    alignas(16) vec3 cameraPos;
    alignas(16) vec3 cameraDir;
    alignas(8)  vec2 screenSize;
};

struct JEGlobalLightingBufferObject {
    alignas(16) vec3 sunDirection;
    alignas(16) vec3 sunColor;
    alignas(16) vec3 ambient;
    alignas(16) vec3 clearColor;
    alignas(8)  vec2 fogPlanes;
};

void init(const char* windowName, int width, int height, JEGraphicsSettings settings);
void mainLoop();
void deinit();
void registerOnUpdate(void (*function)(double dt));
void registerOnKey(void (*function)(int key, bool pressed, double dt));
void putGameObject(const std::string& name, const GameObject& g);
std::unordered_map<std::string, GameObject>* getGameObjects();
void putImGuiCall(void (*argument)());
GameObject& getGameObject(const std::string& name);
void deleteGameObject(const std::string& name);
void setFOV(float n);
float getFOV();
int getCurrentWidth();
int getCurrentHeight();
Transform* cameraAccess();
bool isKeyDown(int key);
bool isMouseButtonDown(int button);
GLFWwindow** getWindow();
#ifdef _MSC_VER
// If MSVC is our compiler
// then for some reason the vec2 (vec<2, float, 0>) get turned into vec<2, float, 3>
// So we need to assign alternate functions to fix this.
vec2_MSVC getRawCursorPos();
vec2_MSVC getCursorPos();
void setRawCursorPos(vec2_MSVC pos);
void setSunProperties(vec3_MSVC position, vec3_MSVC color);
#else
vec2 getRawCursorPos();
vec2 getCursorPos();
void setRawCursorPos(vec2 pos);
void setSunProperties(vec3 position, vec3 color);
#endif
void setCursorPos(vec2 pos);
unsigned int createTexture(const std::string& name, const std::string& fileName);
unsigned int getTexture(const std::string& name);
bool textureExists(const std::string &name);
unsigned int getProgram(const std::string& name);
void registerProgram(const std::string& name, const std::string& vertex, const std::string& fragment, const JEShaderProgramSettings& settings);
double getFrameTime();
double getUpdateTime();
int getFPS();
size_t getRenderableCount();
void setMouseVisible(bool vis);
void setAmbient(float r, float g, float b);
void setAmbient(vec3 rgb);
void registerOnMouse(void (*function)(int button, bool pressed, double dt));
void setSkyboxEnabled(bool enabled);
void setClippingPlanes(vec2 near_far);
unsigned int createUniformBuffer(size_t bufferSize);
void setClearColor(float r, float g, float b);
void setClearColor(glm::vec3 rgb);
void setFogPlanes(float near, float far);
void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll);
unsigned int getUBOID();
unsigned int getLBOID();
void setTextureFilterMode(JETextureFilter filter);
void clearGameObjects();
bool* runUpdatesAccess();
bool* runObjectUpdatesAccess();
bool* forceSkipUpdateAccess();
#ifdef DEBUG_ENABLED
std::unordered_map<std::string, unsigned int> getTexs();
std::string textureReverseLookup(unsigned int num);
std::string programReverseLookup(unsigned int num);
#endif

#endif //JOSHENGINE_ENGINE_H
