//
// Created by Ember Lee on 3/9/24.
//
#include "engineconfig.h"
#include "engine.h"

#include "gfx/vk/gfx_vk.h"
#include "gfx/wgpu/gfx_wgpu.h"

#include <iostream>
#include <unordered_map>
#include <queue>

#include <glm/gtx/euler_angles.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace JE {
    [[nodiscard]] mat4 Transform::getRotateMatrix() const {
        const vec3 radianRotation = radians(rotation);
        return eulerAngleXYZ(radianRotation.x, radianRotation.y, radianRotation.z);
    }

    GLFWwindow* window;

    std::vector<void (*)(double dt)> onUpdate;
    std::vector<void (*)(int key, bool pressed)> onKey;
    std::vector<void (*)(int button, bool pressed)> onMouse;
    std::vector<void (*)(ButtonInput button, bool pressed)> onButton;

    std::unordered_map<std::string, GameObject> gameObjects = {};

    Renderable skybox;
    Transform camera(vec3(0, 0, 5), vec3(180, 0, 0), vec3(1));
    vec2 clippingPlanesPerspective{0.01f, 500.0f};

    bool keys[GLFW_KEY_LAST];
    bool mouseButtons[GLFW_MOUSE_BUTTON_8-GLFW_MOUSE_BUTTON_1];

    bool lastButtonValues[BUTTON_INPUTS_MAX_ENUM];
    uint16_t buttonInputs[BUTTON_INPUTS_MAX_ENUM];

    std::unordered_map<std::string, unsigned int> programs;
    std::unordered_map<std::string, unsigned int> textures;

    std::vector<void (*)()> imGuiCalls;

    size_t renderableCount = 0;

    bool drawSkybox;
    bool skyboxSupported;

    vec3 sunDirection(0, 1, 0);
    vec3 sunColor(0, 0, 0);
    vec3 ambient(0);
    vec3 clearColor;
    vec2 fogPlanes(0, 200);

    unsigned int uboID{};
    unsigned int lboID{};

    TextureFilter currentFilterMode = LINEAR;

    bool runUpdates = true;
    bool runObjectUpdates = true;
    bool forceSkipUpdate = false;

    bool* runUpdatesAccess() {return &runUpdates;}
    bool* runObjectUpdatesAccess() {return &runObjectUpdates;}
    bool* forceSkipUpdateAccess() {return &forceSkipUpdate;}
    void  skipUpdate() { forceSkipUpdate = true; }

    unsigned int getUBOID() {
        return uboID;
    }

    unsigned int getLBOID() {
        return lboID;
    }

#ifdef DEBUG_ENABLED
    std::unordered_map<std::string, unsigned int> getTexs() {
        return textures;
    }
    std::string programReverseLookup(const unsigned int num){
        for (const auto& i : programs){
            if (i.second == num) return i.first;
        }
        return "";
    }

    std::string textureReverseLookup(const unsigned int num){
        for (const auto& i : textures){
            if (i.second == num) return i.first;
        }
        return "";
    }
#endif

    void setClippingPlanes(float near, float far) {
        clippingPlanesPerspective = {near, far};
    }


    void setSkyboxEnabled(const bool enabled) {
        drawSkybox = enabled && skyboxSupported;
    }

    void recopyLightingBuffer() {
        GlobalLightingBufferObject lighting_buffer = {sunDirection, sunColor, ambient, clearColor, fogPlanes};
        updateUniformBuffer(lboID, &lighting_buffer, sizeof(GlobalLightingBufferObject), true);
    }

    void setAmbient(const vec3 rgb) {
        ambient = rgb;
        recopyLightingBuffer();
    }

    void setAmbient(const float r, const float g, const float b){
        setAmbient(vec3(r, g, b));
    }

    void setClearColor(const float r, const float g, const float b) {
        clearColor = vec3(r, g, b);
        recopyLightingBuffer();
        GFX::setClearColor(r, g, b);
    }

    void setFogPlanes(float near, float far) {
        fogPlanes = {near, far};
        recopyLightingBuffer();
    }

#ifdef __EMSCRIPTEN__
    bool emscriptenRequestPointer = false;
#endif

    void setMouseVisible(const bool vis) {
        if (vis) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#ifdef __EMSCRIPTEN__
            emscriptenRequestPointer = false;
            emscripten_exit_pointerlock();
#endif
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#ifdef __EMSCRIPTEN__
            emscriptenRequestPointer = true;
#endif
        }
    }

    size_t getRenderableCount() {
        return renderableCount;
    }

    int windowWidth, windowHeight;

    double frameTime = 0;
    double updateTime = 0;
    int fps = 0;

    double getFrameTime() {
        return frameTime;
    }

    double getUpdateTime() {
        return updateTime;
    }

    int getFPS() {
        return fps;
    }

    std::unordered_map<std::string, GameObject>* getGameObjects() {
        return &gameObjects;
    }

    void clearGameObjects() {
        gameObjects = {};
        skipUpdate(); // prevent the gameobject update loop from accessing a null reference
    }

    void putImGuiCall(void (*argument)()) {
        imGuiCalls.push_back(argument);
    }

    void createSkyboxShader() {
        const unsigned int vertID = GFX::loadShader("./shaders/skybox_vertex.glsl", JE_VERTEX_SHADER);
        const unsigned int fragID = GFX::loadShader("./shaders/skybox_fragment.glsl", JE_FRAGMENT_SHADER);
        programs.insert({"skybox", GFX::createProgram(vertID, fragID,
                                                        {false,
                                                         true,
                                                         false,
                                                         false,
                                                         (ShaderInputBit::Uniform | (ShaderInputBit::Texture << 1)),
                                                         2},
                                                      VERTEX
                // You may be thinking "What the fuck does this do?"
                // "Why is this here?"
                // WGPU is a bitchass motherfucker. That's why.
    #ifdef GFX_API_WEBGPU
                                                    , false
    #endif
                                                      )});
    }

    void createShader(const std::string& name, const std::string& vertex, const std::string& fragment, const ShaderProgramSettings& settings, const VertexType& vtype) {
        unsigned int vertID = GFX::loadShader(vertex, JE_VERTEX_SHADER);
        unsigned int fragID = GFX::loadShader(fragment, JE_FRAGMENT_SHADER);
        programs.insert({name, GFX::createProgram(vertID, fragID, settings, vtype)});
    }

    unsigned int getShader(const std::string& name) {
        return programs.at(name);
    }

    void setTextureFilterMode(const TextureFilter filter) {
        currentFilterMode = filter;
    }

    unsigned int createTexture(const std::string& name, const std::string& filePath) {
        unsigned int id;
        try {
            id = GFX::loadTexture(filePath, currentFilterMode);
        } catch ([[maybe_unused]] std::runtime_error &e) {
            id = textures.at("missing");
            std::cerr << "Failed to create \"" << name << "\" from file at path " << filePath << "! Texture ID will be set to missing." << std::endl;
        }
        textures.insert({name, id});
        return id;
    }

    unsigned int createTexture(const std::string& name, const std::string& filePath, const std::string& bundleFilePath) {
        unsigned int id;
        try {
            std::vector<unsigned char> file = getFileCharVec(filePath, bundleFilePath);
            id = GFX::loadBundledTexture(reinterpret_cast<char *>(&file[0]), file.size(), currentFilterMode);
        } catch ([[maybe_unused]] std::runtime_error &e) {
            id = textures.at("missing");
            std::cerr << "Failed to create \"" << name << "\" from file at path " << filePath << "! Texture ID will be set to missing." << std::endl;
        }
        textures.insert({name, id});
        return id;
    }

    bool textureExists(const std::string &name) {
        return textures.count(name);
    }

    unsigned int getTexture(const std::string& name) {
        if (!textureExists(name)) {
            std::cerr << "Texture \"" + name + "\" not found, defaulting to missing_tex.png" << std::endl;
            return textures.at("missing");
        }
        return textures.at(name);
    }

    GLFWwindow** getWindow() {
        return &window;
    }

    bool isKeyDown(const int key) {
        return keys[key];
    }

    bool isMouseButtonDown(const int button) {
        return mouseButtons[button];
    }

    bool isButtonDown(const ButtonInput button) {
        // TODO: Controllers in general. D-Pads, Stick "button" emulation
        // Since GLFW keyboards only register under <512 keys a 16 bit integer gives us leeway to do whatever really.
        if (const uint16_t code = buttonInputs[button]; code & 0xF000) {
            return isMouseButtonDown(code & 0x0FFF);
        } else {
            return isKeyDown(code & 0x0FFF);
        }
    }

    vec2 getRawCursorPos() {
        double xpos, ypos;
        glfwGetCursorPos(window, (&xpos), (&ypos));
        return {xpos, ypos};
    }
    vec2 getCursorPos() {
        vec2 cpos = getRawCursorPos();
        // modify to -1, 1 coordinate space
        cpos /= vec2(getCurrentWidth(), -getCurrentHeight());
        cpos += vec2(-0.5, 0.5);
        cpos *= 2;

        // scale Y axis by the current scaled height
        cpos /= vec2(1, static_cast<float>(getCurrentWidth())  * 1.0f / static_cast<float>(getCurrentHeight()));
        cpos *= vec2(10.0, 10.0);
        return cpos;
    }

    void setRawCursorPos(const vec2 pos) {
        glfwSetCursorPos(window, pos.x, pos.y);
    }

    void setSunProperties(const vec3 position, const vec3 color){
        sunDirection = position;
        sunColor = color;
        recopyLightingBuffer();
    }

    void setCursorPos(vec2 pos) {
        // inverse of getCursorPos's coordinate transformation
        pos /= vec2(10.0, 10.0);
        pos *= vec2(1, static_cast<float>(getCurrentWidth()) * (1.0f / static_cast<float>(getCurrentHeight())));
        pos /= 2;
        pos -= vec2(-0.5, 0.5);
        pos *= vec2(getCurrentWidth(), -getCurrentHeight());
        setRawCursorPos(pos);
    }

    unsigned int createUniformBuffer(const size_t bufferSize) {
        return GFX::createUniformBuffer(bufferSize);
    }

    void updateUniformBuffer(const unsigned int id, void* ptr, const size_t size, const bool updateAll) {
        GFX::updateUniformBuffer(id, ptr, size, updateAll);
    }

    void registerOnUpdate(void (*function)(double dt)) {
        onUpdate.push_back(function);
    }

    void registerOnKey(void (*function)(int key, bool pressed)) {
        onKey.push_back(function);
    }

    void registerOnMouse(void (*function)(int button, bool pressed)) {
        onMouse.push_back(function);
    }

    void registerOnButton(void (*function)(ButtonInput button, bool pressed)) {
        onButton.push_back(function);
    }

    void bindButton(const ButtonInput button, const int id, const bool isMouse) {
        buttonInputs[button] = id | (isMouse ? 0xF000 : 0x0000);
    }

    GameObject& putGameObject(const std::string& name, const GameObject& g) {
        return gameObjects.insert({name, g}).first->second;
    }

    GameObject& getGameObject(const std::string& name) {
        return gameObjects.at(name);
    }

    void deleteGameObject(const std::string& name) {
        gameObjects.erase(name);
    }

    int getCurrentWidth() {
        return windowWidth;
    }

    int getCurrentHeight() {
        return windowHeight;
    }

    void framebuffer_size_callback(GLFWwindow* windowInstance,[[maybe_unused]] int unused1, [[maybe_unused]] int unused2) {
        // Because of displays like Apple's Retina having multiple pixel values per pixel (or some bs like that)
        // the framebuffer is not always the window size. We need to make sure the real window size is saved.
        glfwGetWindowSize(windowInstance, &windowWidth, &windowHeight);
        GFX::resizeViewport();
    }

#ifdef __EMSCRIPTEN__
    bool emscripten_click_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
        if (emscriptenRequestPointer) emscripten_request_pointerlock("canvas", false);
        return true;
    }
#endif

    void init(const char* windowName, const int width, const int height, GraphicsSettings graphicsSettings) {
        std::cout << "JoshEngine " << ENGINE_VERSION_STRING << std::endl;
        std::cout << "Starting engine init." << std::endl;

        bindButton(FORWARD, GLFW_KEY_W, false);
        bindButton(BACKWARD, GLFW_KEY_S, false);
        bindButton(LEFT, GLFW_KEY_A, false);
        bindButton(RIGHT, GLFW_KEY_D, false);
        bindButton(UP, GLFW_KEY_SPACE, false);
        bindButton(DOWN, GLFW_KEY_LEFT_SHIFT, false);
        bindButton(UI_SELECT, GLFW_MOUSE_BUTTON_LEFT, true);
        bindButton(UI_EXIT, GLFW_KEY_ESCAPE, false);

        windowWidth = width;
        windowHeight = height;

        ambient = {max(graphicsSettings.clearColor[0] - 0.5f, 0.1f), max(graphicsSettings.clearColor[1] - 0.5f, 0.1f), max(graphicsSettings.clearColor[2] - 0.5f, 0.1f)};
        clearColor = vec3(graphicsSettings.clearColor[0], graphicsSettings.clearColor[1], graphicsSettings.clearColor[2]);

        GFX::init(&window, windowName, width, height, graphicsSettings);

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
#ifdef __EMSCRIPTEN__
        emscripten_set_click_callback("canvas", nullptr, true, emscripten_click_callback);
#endif

        uboID = createUniformBuffer(sizeof(UniformBufferObject));
        lboID = createUniformBuffer(sizeof(GlobalLightingBufferObject));

        // Missing texture init
        createTexture("missing", "./textures/missing_tex.png");
        if (!textures.count("missing")) {
            std::cerr << "Essential engine file missing." << std::endl;
            exit(1);
        }

        drawSkybox = graphicsSettings.skybox;
        skyboxSupported = graphicsSettings.skybox;

        if (graphicsSettings.skybox) {
            // Skybox init
            createSkyboxShader();
            skybox = loadObj("./models/skybox.obj", getShader("skybox"), {uboID, GFX::loadCubemap({
                                                 "./skybox/px_right.jpg",
                                                 "./skybox/nx_left.jpg",
                                                 "./skybox/py_up.jpg",
                                                 "./skybox/ny_down.jpg",
                                                 "./skybox/nz_front.jpg",
                                                 "./skybox/pz_back.jpg"
                                         })})[0];
            if (!skybox.enabled()) {
                std::cerr << "Essential engine file missing." << std::endl;
                exit(1);
            }
        }
        std::cout << "Graphics init successful!" << std::endl;

        Audio::init();
        std::cout << "Audio init successful!" << std::endl;
        UI::init();
        std::cout << "UI init successful!" << std::endl;
#ifdef DEBUG_ENABLED
        initDebugTools();
        std::cout << "Debug init successful!" << std::endl;
#endif // DEBUG_ENABLED
    }

    void deinit() {
        GFX::deinit();
    }

    float fov = 78.0f;

    void setFOV(const float n) {
        fov = n;
    }
    float getFOV() {
        return fov;
    }

    Transform* cameraAccess() {
        return &camera;
    }

    auto compareLambda = [](std::pair<double, Renderable *>& left, const std::pair<double, Renderable*>& right){return left.first < right.first;};
    double currentTime, lastTime, lastTimesCheck, frameDrawStart, updateStart, lastFPSUpdateTime;
    int currentFPSCtr;

    void tick() {
        currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (currentTime - lastFPSUpdateTime > 1){
            lastFPSUpdateTime = currentTime;
            fps = currentFPSCtr;
            currentFPSCtr = 0;
        }

        bool doTimesCheck = currentTime - lastTimesCheck > 0.1;
        if (doTimesCheck) {
            lastTimesCheck = glfwGetTime();
            updateStart = glfwGetTime()*1000;
        }

        for (int keyActionIter = 0; keyActionIter < GLFW_KEY_LAST; keyActionIter++) {
            if (bool current = glfwGetKey(window, keyActionIter) == GLFW_PRESS; keys[keyActionIter] != current) {
                keys[keyActionIter] = current;
                for (auto & onKeyFunction : onKey) {
                    onKeyFunction(keyActionIter, current);
                }
            }
        }

        for (int mouseActionIter = 0; mouseActionIter < 7; mouseActionIter++){
            if (bool current = glfwGetMouseButton(window, mouseActionIter+GLFW_MOUSE_BUTTON_1) == GLFW_PRESS; mouseButtons[mouseActionIter] != current) {
                mouseButtons[mouseActionIter] = current;
                for (auto & onMouseFunction : onMouse) {
                    onMouseFunction(mouseActionIter+GLFW_MOUSE_BUTTON_1, current);
                }
            }
        }

        for (int buttonIter = 0; buttonIter < BUTTON_INPUTS_MAX_ENUM; buttonIter++) {
            const auto input = static_cast<ButtonInput>(buttonIter);
            if (const bool pressed = isButtonDown(input); pressed != lastButtonValues[buttonIter]) {
                lastButtonValues[buttonIter] = pressed;
                for (auto & onButtonFunction : onButton) {
                    onButtonFunction(input, pressed);
                }
            }
        }

        if (runUpdates && !forceSkipUpdate) {
            for (auto &onUpdateFunction: onUpdate) {
                onUpdateFunction(deltaTime);
                if (forceSkipUpdate) break;
            }
        }

        if (runObjectUpdates && !forceSkipUpdate) {
            for (auto &g: gameObjects) {
                for (auto &gameObjectFunction: g.second.onUpdate) {
                    gameObjectFunction(deltaTime, &g.second);
                    if (forceSkipUpdate) break;
                }
                if (forceSkipUpdate) break;
            }
        }

        forceSkipUpdate = false;

        // Right vector
        vec3 right = vec3(
                sin(radians(camera.rotation.x - 90)),
                0,
                cos(radians(camera.rotation.x - 90))
        );

        vec3 up = cross( right, camera.direction() )
        * mat3(rotate(mat4(1), radians(camera.rotation.z), camera.direction()));

        // Camera matrix
        mat4 cameraMatrix = lookAt(
                camera.position, // camera is at its position
                camera.position+camera.direction(), // looks in look direction
                up  // up vector
        );

        Audio::updateListener(camera.position, vec3(0), camera.direction(), up);

        if (doTimesCheck) {
            updateTime = glfwGetTime()*1000 - updateStart;
            frameDrawStart = glfwGetTime()*1000;
        }

        std::vector<Renderable*> renderables;
        // We're going to guess that we have around the same amount of renderables for this frame.
        renderables.reserve(renderableCount);
        std::priority_queue<std::pair<double, Renderable*>, std::deque<std::pair<double, Renderable*>>, decltype(compareLambda)> transparentWorldRenderables;
        std::priority_queue<std::pair<double, Renderable*>, std::deque<std::pair<double, Renderable*>>, decltype(compareLambda)> uiRenderables;

        renderableCount = 0;

        if (drawSkybox) {
            skybox.setMatrices(camera.getTranslateMatrix(), glm::identity<mat4>(), glm::identity<mat4>());
            renderables.push_back(&skybox);
            renderableCount++;
        }

        for (auto& item : gameObjects) {
            for (auto& r : item.second.renderables) {
                if (r.enabled()) {
                    renderableCount++;
                    r.setMatrices(item.second.transform.getTranslateMatrix(), item.second.transform.getRotateMatrix(), item.second.transform.getScaleMatrix());
                    if (r.checkUIBit()) {
                        uiRenderables.emplace(-item.second.transform.position.z, &r); // I don't want to write a second lambda
                    } else if (r.manualDepthSort()) {
                        transparentWorldRenderables.emplace(distance(camera.position, item.second.transform.position), &r);
                    }
                    else {
                        renderables.push_back(&r);
                    }
                }
            }
        }

        size_t a = transparentWorldRenderables.size();
        for (int i = 0; i < a; i++){
            auto [_, r] = transparentWorldRenderables.top();
            renderables.push_back(r);
            transparentWorldRenderables.pop();
        }

        a = uiRenderables.size();
        for (int i = 0; i < a; i++){
            auto [_, r] = uiRenderables.top();
            renderables.push_back(r);
            uiRenderables.pop();
        }

        float scaledHeight = static_cast<float>(windowHeight) * (1.0f / static_cast<float>(windowWidth));
        float scaledWidth = 1.0f;

        UniformBufferObject ubo = {
            cameraMatrix,
            ortho(-scaledWidth*10,scaledWidth*10,-scaledHeight*10,scaledHeight*10,-1.0f,1.0f),
            perspective(radians(fov), (float) windowWidth / (float) windowHeight, clippingPlanesPerspective.x, clippingPlanesPerspective.y),
            camera.position,
            camera.direction(),
            {windowWidth, windowHeight}
        };

        updateUniformBuffer(uboID, &ubo, sizeof(UniformBufferObject), false);

        GFX::renderFrame(renderables, imGuiCalls);
        ++currentFPSCtr;

        if (doTimesCheck)
            frameTime = glfwGetTime()*1000 - frameDrawStart;

        glfwPollEvents();
    }

    void mainLoop() {
        currentTime = glfwGetTime();
        lastTime = currentTime;
        lastTimesCheck = currentTime;
        frameDrawStart = currentTime;
        lastFPSUpdateTime = currentTime;
        currentFPSCtr = 0;
#ifndef __EMSCRIPTEN__
        while (glfwWindowShouldClose(window) == 0) {
            tick();
        }
#else
        emscripten_set_main_loop_arg([](void *arg) {tick();}, nullptr, 0, true);
#endif
    }
}