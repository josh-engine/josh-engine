//
// Created by Ember Lee on 3/9/24.
//
#include "engineconfig.h"
#include "gfx/vk/gfx_vk.h"
#include "engine.h"
#include <iostream>
#include <unordered_map>
#include <queue>

#include <glm/gtx/euler_angles.hpp>

namespace JE {
[[nodiscard]] mat4 Transform::getRotateMatrix() const {
    vec3 radianRotation = radians(rotation);
    return glm::eulerAngleXYZ(radianRotation.x, radianRotation.y, radianRotation.z);
}

GLFWwindow* window;

std::vector<void (*)(double dt)> onUpdate;
std::vector<void (*)(int key, bool pressed, double dt)> onKey;
std::vector<void (*)(int button, bool pressed, double dt)> onMouse;

std::unordered_map<std::string, GameObject> gameObjects = {};

Renderable skybox;
Transform camera(glm::vec3(0, 0, 5), glm::vec3(180, 0, 0), glm::vec3(1));
vec2 clippingPlanesPerspective{0.01f, 500.0f};

bool keys[GLFW_KEY_LAST];
bool mouseButtons[GLFW_MOUSE_BUTTON_8-GLFW_MOUSE_BUTTON_1];

std::unordered_map<std::string, unsigned int> programs;
std::unordered_map<std::string, unsigned int> textures;

std::vector<void (*)()> imGuiCalls;

size_t renderableCount = 0;

bool drawSkybox;
bool skyboxSupported;

glm::vec3 sunDirection(0, 1, 0);
glm::vec3 sunColor(0, 0, 0);
glm::vec3 ambient(0);
glm::vec3 clearColor;
glm::vec2 fogPlanes(0, 200);

unsigned int uboID{};
unsigned int lboID{};

TextureFilter currentFilterMode = JE_TEXTURE;

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
std::string programReverseLookup(unsigned int num){
    for (const auto& i : programs){
        if (i.second == num) return i.first;
    }
    return "";
}

std::string textureReverseLookup(unsigned int num){
    for (const auto& i : textures){
        if (i.second == num) return i.first;
    }
    return "";
}
#endif

void setClippingPlanes(float near, float far) {
    clippingPlanesPerspective = {near, far};
}


void setSkyboxEnabled(bool enabled) {
    drawSkybox = enabled && skyboxSupported;
}

void recopyLightingBuffer() {
    GlobalLightingBufferObject lighting_buffer = {sunDirection, sunColor, ambient, clearColor, fogPlanes};
    updateUniformBuffer(lboID, &lighting_buffer, sizeof(GlobalLightingBufferObject), true);
}

void setAmbient(glm::vec3 rgb) {
    ambient = rgb;
    recopyLightingBuffer();
}

void setAmbient(float r, float g, float b){
    setAmbient(glm::vec3(r, g, b));
}

void setClearColor(float r, float g, float b) {
    clearColor = vec3(r, g, b);
    recopyLightingBuffer();
    VK::setClearColor(r, g, b);
}

void setFogPlanes(float near, float far) {
    fogPlanes = {near, far};
    recopyLightingBuffer();
}

void setMouseVisible(bool vis) {
    // Disabled-2 = normal, so if visible is true subtract 2 from mode to get normal.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED-((int)vis)*2);
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

void createShader(const std::string& name, const std::string& vertex, const std::string& fragment, const ShaderProgramSettings& settings, const VertexType& vtype) {
    unsigned int vertID = VK::loadShader(vertex, JE_VERTEX_SHADER);
    unsigned int fragID = VK::loadShader(fragment, JE_FRAGMENT_SHADER);
    programs.insert({name, VK::createProgram(vertID, fragID, settings, vtype)});
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
        id = VK::loadTexture(filePath, currentFilterMode);
    } catch (std::runtime_error &e) {
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
        id = VK::loadBundledTexture(reinterpret_cast<char *>(&file[0]), file.size(), currentFilterMode);
    } catch (std::runtime_error &e) {
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

bool isKeyDown(int key) {
    return keys[key];
}

bool isMouseButtonDown(int button) {
    return mouseButtons[button];
}

glm::vec2 getRawCursorPos() {
    double xpos, ypos;
    glfwGetCursorPos(window, (&xpos), (&ypos));
    return {xpos, ypos};
}
glm::vec2 getCursorPos() {
    glm::vec2 cpos = getRawCursorPos();
    // modify to -1, 1 coordinate space
    cpos /= glm::vec2(getCurrentWidth(), -getCurrentHeight());
    cpos += glm::vec2(-0.5, 0.5);
    cpos *= 2;

    // scale Y axis by the current scaled height
    cpos /= glm::vec2(1, static_cast<float>(getCurrentWidth())  * 1.0f / static_cast<float>(getCurrentHeight()));
    cpos *= glm::vec2(10.0, 10.0);
    return cpos;
}
void setRawCursorPos(glm::vec2 pos) {
    glfwSetCursorPos(window, pos.x, pos.y);
}
void setSunProperties(glm::vec3 position, glm::vec3 color){
    sunDirection = position;
    sunColor = color;
    recopyLightingBuffer();
}
#endif

void setCursorPos(glm::vec2 pos) {
    // inverse of getCursorPos's coordinate transformation
    pos /= glm::vec2(10.0, 10.0);
    pos *= glm::vec2(1, static_cast<float>(getCurrentWidth()) * (1.0f / static_cast<float>(getCurrentHeight())));
    pos /= 2;
    pos -= glm::vec2(-0.5, 0.5);
    pos *= glm::vec2(getCurrentWidth(), -getCurrentHeight());
    setRawCursorPos(pos);
}

unsigned int createUniformBuffer(size_t bufferSize) {
    return VK::createUniformBuffer(bufferSize);
}

void updateUniformBuffer(unsigned int id, void* ptr, size_t size, bool updateAll) {
    VK::updateUniformBuffer(id, ptr, size, updateAll);
}

void registerOnUpdate(void (*function)(double dt)) {
    onUpdate.push_back(function);
}

void registerOnKey(void (*function)(int key, bool pressed, double dt)) {
    onKey.push_back(function);
}

void registerOnMouse(void (*function)(int button, bool pressed, double dt)) {
    onMouse.push_back(function);
}

void putGameObject(const std::string& name, const GameObject& g) {
    gameObjects.insert({name, g});
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
    VK::resizeViewport();
}

void init(const char* windowName, int width, int height, GraphicsSettings graphicsSettings) {
    std::cout << "JoshEngine " << ENGINE_VERSION_STRING << std::endl;
    std::cout << "Starting engine init." << std::endl;

    windowWidth = width;
    windowHeight = height;

    ambient = {glm::max(graphicsSettings.clearColor[0] - 0.5f, 0.1f), glm::max(graphicsSettings.clearColor[1] - 0.5f, 0.1f), glm::max(graphicsSettings.clearColor[2] - 0.5f, 0.1f)};
    clearColor = vec3(graphicsSettings.clearColor[0], graphicsSettings.clearColor[1], graphicsSettings.clearColor[2]);

    VK::initGFX(&window, windowName, width, height, graphicsSettings);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
        createShader("skybox",
                     "./shaders/skybox_vertex.glsl",
                     "./shaders/skybox_fragment.glsl",
                // hacky bullshit. don't depth test, disable depth writes (transparency mode :skull:)
                     {false, true, false, false, (ShaderInputBit::Uniform | (ShaderInputBit::Texture << 1)), 2});
        skybox = loadObj("./models/skybox.obj", getShader("skybox"), {uboID, VK::loadCubemap({
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
    VK::deinitGFX();
}

float fov = 78.0f;

void setFOV(float n) {
    fov = n;
}
float getFOV() {
    return fov;
}

Transform* cameraAccess() {
    return &camera;
}

auto compareLambda = [](std::pair<double, Renderable *>& left, const std::pair<double, Renderable*>& right){return left.first < right.first;};

void mainLoop() {
    double currentTime = glfwGetTime();
    double lastTime = currentTime;
    double lastTimesCheck = currentTime;
    double frameDrawStart = currentTime;
    double updateStart;
    double lastFPSUpdateTime = currentTime;
    int currentFPSCtr = 0;
    while (glfwWindowShouldClose(window) == 0) {
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
            bool current = glfwGetKey(window, keyActionIter) == GLFW_PRESS;
            if (keys[keyActionIter] != current) {
                keys[keyActionIter] = current;
                for (auto & onKeyFunction : onKey) {
                    onKeyFunction(keyActionIter, current, deltaTime);
                }
            }
        }

        for (int mouseActionIter = 0; mouseActionIter < 7; mouseActionIter++){
            bool current = glfwGetMouseButton(window, mouseActionIter+GLFW_MOUSE_BUTTON_1) == GLFW_PRESS;
            if (mouseButtons[mouseActionIter] != current) {
                mouseButtons[mouseActionIter] = current;
                for (auto & onMouseFunction : onMouse) {
                    onMouseFunction(mouseActionIter+GLFW_MOUSE_BUTTON_1, current, deltaTime);
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
        glm::vec3 right = glm::vec3(
                sin(glm::radians(camera.rotation.x - 90)),
                0,
                cos(glm::radians(camera.rotation.x - 90))
        );
        glm::vec3 up = glm::cross( right, camera.direction() );

        // Camera matrix
        glm::mat4 cameraMatrix = glm::lookAt(
                camera.position, // camera is at its position
                camera.position+camera.direction(), // looks in look direction
                up  // up vector
        );

        Audio::updateListener(camera.position, glm::vec3(0), camera.direction(), up);

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
                        transparentWorldRenderables.emplace(glm::distance(camera.position, item.second.transform.position), &r);
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
            glm::ortho(-scaledWidth*10,scaledWidth*10,-scaledHeight*10,scaledHeight*10,-1.0f,1.0f),
            glm::perspective(glm::radians(fov), (float) windowWidth / (float) windowHeight, clippingPlanesPerspective.x, clippingPlanesPerspective.y),
            camera.position,
            camera.direction(),
            {windowWidth, windowHeight}
        };

        updateUniformBuffer(uboID, &ubo, sizeof(UniformBufferObject), false);

        VK::renderFrame(renderables, imGuiCalls);
        ++currentFPSCtr;

        if (doTimesCheck)
            frameTime = glfwGetTime()*1000 - frameDrawStart;

        glfwPollEvents();
    }
}
}