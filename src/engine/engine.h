//
// Created by Ember Lee on 3/9/24.
//

#ifndef JOSHENGINE_ENGINE_H
#define JOSHENGINE_ENGINE_H
#include <unordered_map>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gfx/renderable.h"

#include "sound/audioutil.h"
#include "gfx/modelutil.h"
#include "debug/debugutil.h"
#include "jbd/bundleutil.h"
#include "ui/je/uiutil.h"

using namespace glm;

namespace JE {
    namespace ShaderInputBit {
        const char Uniform = 0;
        const char Texture = 1;
    }

    enum TextureFilter {
        JE_PIXEL_ART = 0,
        JE_TEXTURE = 1
    };

    struct GraphicsSettings {
        bool vsyncEnabled;
        bool skybox;
        float clearColor[3];
        int msaaSamples;
    };

    struct ShaderProgramSettings {
        bool testDepth = true;
        bool transparencySupported = false;
        bool doubleSided = false;
        bool depthAlwaysPass = false;

        uint32_t shaderInputs = 0;
        uint8_t shaderInputCount = 0;
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
        std::vector<void (*)(double dt, GameObject *g)> onUpdate = {};
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

    struct UniformBufferObject {
        alignas(16) mat4 view;
        alignas(16) mat4 _2dProj;
        alignas(16) mat4 _3dProj;
        alignas(16) vec3 cameraPos;
        alignas(16) vec3 cameraDir;
        alignas(8)  vec2 screenSize;
    };

    struct GlobalLightingBufferObject {
        alignas(16) vec3 sunDirection;
        alignas(16) vec3 sunColor;
        alignas(16) vec3 ambient;
        alignas(16) vec3 clearColor;
        alignas(8)  vec2 fogPlanes;
    };

/**
 * Do initialization for the engine. THIS IS REQUIRED TO RUN BEFORE CALLING ANY OTHER ENGINE FUNCTIONS.
 * @param windowName Name of the desktop window your game will run in
 * @param width Width of the desktop window your game will run in
 * @param height Height of the desktop window your game will run in
 * @param settings Your GraphicsSettings struct that describes that graphics capabilities the engine should run with.
 */
    void init(const char *windowName, int width, int height, GraphicsSettings settings);

/**
 * Run the engine main loop, and don't stop till we quit, baby!!
 */
    void mainLoop();

/**
 * Not required, but if you CAN call this on a clean exit that'd be nice.
 * The Vulkan spec and in turn your graphics driver likes that more than exit(0).
 */
    void deinit();

/**
 * Register a function to be called every game update/frame.
 * @param function Pointer to the function to be called.
 */
    void registerOnUpdate(void (*function)(double dt));

/**
 * Register a function to be called when a key is updated on the keyboard.
 * @param function Pointer to the function to be called.
 */
    void registerOnKey(void (*function)(int key, bool pressed, double dt));

/**
 * Register a function to be called when the mouse buttons are updated.
 * @param function Pointer to the function to be called.
 */
    void registerOnMouse(void (*function)(int button, bool pressed, double dt));

/**
 * Add a GameObject to the engine's current objects.
 * @param name Name of the GameObject. All GameObject names must be unique, and duplicates will fail to be added with no error message.
 * @param g The GameObject to add.
 */
    void putGameObject(const std::string &name, const GameObject &g);

/**
 * @return A pointer to the engine map of GameObjects.
 */
    std::unordered_map<std::string, GameObject> *getGameObjects();

/**
 * Get a specific GameObject from the map by name.
 * @param name Name of GameObject to get.
 * @return GameObject in the map with that name. A name not found in the map will return a not found value of whatever std::unordered_map should return, because this function is just a wrapper of gameObjects.at(name).
 */
    GameObject &getGameObject(const std::string &name);

/**
 * Delete a GameObject from the map by name.
 * @param name GameObject to delete.
 */
    void deleteGameObject(const std::string &name);

/**
 * Delete ALL GameObjects in the scene.
 * Also skips an update to prevent the existing for loop from trying to execute an invalid function pointer and segfaulting.
 * This will not matter for your purposes because there will be no GameObjects left to worry about.
 * That behavior will not cause errors in your code.
 * Please just use breakpoints. Editing engine files is usually not the solution you are looking for.
 */
    void clearGameObjects();

/**
 * @return Current game window width
 */
    int getCurrentWidth();

/**
 * @return Current game window height
 */
    int getCurrentHeight();

/**
 * Check if a GLFW Key ID is pressed on the keyboard.
 * @param key GLFW Key ID
 * @return Is key down?
 */
    bool isKeyDown(int key);

/**
 * Check if a GLFW Mouse Button ID is pressed on the keyboard.
 * @param key GLFW Mouse Button ID
 * @return Is button down?
 */
    bool isMouseButtonDown(int button);

/**
 * @return A pointer to the GLFW window. You shouldn't need this unless writing your own graphics backend.
 */
    GLFWwindow **getWindow();

    vec2 getRawCursorPos();

    vec2 getCursorPos();

    void setRawCursorPos(vec2 pos);

    void setSunProperties(vec3 position, vec3 color);

    void setCursorPos(vec2 pos);

    void setMouseVisible(bool vis);

/**
 * Create texture in GPU memory with a Descriptor ID.
 * @param name Texture name to be accessed by
 * @param fileName File name to load texture from
 * @return Texture Descriptor ID
 */
    unsigned int createTexture(const std::string &name, const std::string &fileName);

/**
 * Create texture in GPU memory with a Descriptor ID.
 * @param name Texture name to be accessed by
 * @param fileName Alias to load texture from in bundle
 * @param bundleFilePath Bundle to load bytes from
 * @return Texture Descriptor ID
 */
    unsigned int createTexture(const std::string &name, const std::string &filePath, const std::string &bundleFilePath);

/**
 * Get texture Descriptor ID
 * @param name Texture name to retrieve descriptor ID of
 * @return Texture Descriptor ID
 */
    unsigned int getTexture(const std::string &name);

/**
 * Check if a texture name exists.
 * You shouldn't need this unless either
 * 1. you are dumb (my brother in christ you named the texture)
 * 2. you need a sanity check (you cannot this at compile time in a #define by the way)
 * 3. you are doing something weird with engine files, in which case good luck.
 * @param name Texture name to look up
 * @return Texture name maps to a valid ID?
 */
    bool textureExists(const std::string &name);

/**
 * Set the texture filter mode of the engine.
 * JE_PIXEL_ART (closest) is used for well, pixel art, and JE_TEXTURE (bilinear) is used for anything else.
 * Due to the syntax, it may seem like this is a "set only once" thing- which normally it is- but it does not have to be.
 * @param filter Texture filter mode
 */
    void setTextureFilterMode(TextureFilter filter);

/**
 * Create a shader program on the GPU.
 * @param name Name to refer to the shader program with.
 * @param vertex Vertex shader file name. Vulkan's supported types are GLSL and SPV.
 * @param fragment Fragment shader file name. Vulkan's supported types are GLSL and SPV.
 * @param settings The shader program's settings and parameters.
 * @param vtype The type of vertex the Vertex Shader is meant to consume.
 */
    void createShader(const std::string &name, const std::string &vertex, const std::string &fragment,
                      const ShaderProgramSettings &settings, const VertexType &vtype = VERTEX);

/**
 * Get a shader program's ID. This should be put in a Renderable's shader program parameter.
 * @param name Name of the shader program to look up.
 * @return Shader program's ID.
 */
    unsigned int getShader(const std::string &name);

// These are various engine statistics. Pretty self-explanatory.
// Compile with CMake debug and check the stats menu's tooltips for more information.

    double getFrameTime();

    double getUpdateTime();

    int getFPS();

/**
 * @return A pointer to the Transform of the game camera. Scale is unused, and pretty much free real estate, but useful for a first person player box collider.
 */
    Transform *cameraAccess();

/**
 * Set the current perspective view frustum/3d camera's FOV.
 * @param n Camera FOV
 */
    void setFOV(float n);

/**
 * @return Current perspective view frustum/3d camera's FOV
 */
    float getFOV();

/**
 * Set main framebuffer's clear color, and update the value in LBO.
 * To set the framebuffer's clear color *without* updating the LBO (which you should never have to do) use vk_setClearColor in gfx_vk.h
 * @param r Clear color's red component
 * @param g Clear color's green component
 * @param b Clear color's blue component
 */
    void setClearColor(float r, float g, float b);

/**
 * Update the LBO's ambient lighting value.
 * @param r Ambient color's red component
 * @param g Ambient color's green component
 * @param b Ambient color's blue component
 */
    void setAmbient(float r, float g, float b);

/**
 * Set the LBO's fog plane values.
 * @param near Fog's start, or near plane
 * @param far Fog's end/complete obscuration distance, or far plane
 */
    void setFogPlanes(float near, float far);

/**
 * Enable or disable rendering of the skybox.
 * This only matters if your GraphicsSettings skybox value is true when passed to the engine's init().
 * @param enabled Enable or disable rendering the skybox cubemap
 */
    void setSkyboxEnabled(bool enabled);

/**
 * Set the engine's view frustum clipping planes.
 * @param near Near clipping plane
 * @param far Far clipping plane
 */
    void setClippingPlanes(float near, float far);

/**
 * Creates a uniform buffer on the GPU.
 * This is a passthrough to the current graphics API, which at the moment can only be Vulkan.
 * See gfx_vk.cpp for implementation if you need it for some reason.
 * @param bufferSize The size of the buffer in bytes. Recommended to pass a sizeof(some_struct) for easy usage.
 * @return Descriptor ID of the uniform buffer. Use this value in a Renderable's descriptor vector to use it in a shader.
 */
    unsigned int createUniformBuffer(size_t bufferSize);

/**
 * Update a uniform buffer's data on the GPU.
 * @param id Descriptor ID of the uniform buffer to update.
 * @param ptr A pointer to the start of the data you are putting in the buffer.
 * @param size The size of the data you are putting in the buffer.
 * @param updateAll Set the buffer to update across all frames-in-flight. If you are setting this value to persist across multiple frames, set this to true. If you are setting this value every update/frame, set this to false.
 */
    void updateUniformBuffer(unsigned int id, void *ptr, size_t size, bool updateAll);

/**
 * Get the descriptor ID of the engine's Uniform Buffer.
 * @return JoshEngine UBO Descriptor ID
 */
    unsigned int getUBOID();

/**
 * Get the descriptor ID of the engine's Global Light Buffer.
 * @return JoshEngine LBO Descriptor ID
 */
    unsigned int getLBOID();

// Engine loop skips
/**
 * @return A pointer to the engine's run updates flag. Accessible from debug menu.
 */
    bool *runUpdatesAccess();

/**
 * @return A pointer to the engine's run GameObject updates flag. Accessible from debug menu.
 */
    bool *runObjectUpdatesAccess();

/**
 * @return A pointer to the engine's temporary update skip flag.
 */
    bool *forceSkipUpdateAccess();

/**
 * A function to skip the current update.
 * Equivalent to dereferencing the pointer from forceSkipUpdateAccess and setting it to true.
 */
    void skipUpdate();

// Debug functions (ImGui access still allowed on release, just cause it's useful)
// Not documented bc these should not be used in production.
// I didn't tell you how to use them. Don't use them.

#ifdef DEBUG_ENABLED

    size_t getRenderableCount();

    std::unordered_map<std::string, unsigned int> getTexs();

    std::string textureReverseLookup(unsigned int num);

    std::string programReverseLookup(unsigned int num);

#endif

    void putImGuiCall(void (*argument)());
} // namespace je
#endif //JOSHENGINE_ENGINE_H