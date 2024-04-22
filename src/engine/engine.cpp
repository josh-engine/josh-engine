//
// Created by Ember Lee on 3/9/24.
//
#include "engineconfig.h"
#include "gfx/opengl/gfx_gl41.h"
#include "gfx/vk/gfx_vk.h"
#include "sound/engineaudio.h"
#include "engine.h"
#include <iostream>
#include <unordered_map>
#include <map>
#ifdef GFX_API_VK
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <queue>
#include "gfx/modelutil.h"
#include "enginedebug.h"

GLFWwindow* window;
std::vector<void (*)(double dt)> onUpdate;
std::vector<void (*)(int key, bool pressed, double dt)> onKey;
std::vector<void (*)(int button, bool pressed, double dt)> onMouse;
std::unordered_map<std::string, GameObject> gameObjects;
Transform camera;
bool keys[GLFW_KEY_LAST];
bool mouseButtons[GLFW_MOUSE_BUTTON_8-GLFW_MOUSE_BUTTON_1];
Renderable skybox;
std::unordered_map<std::string, unsigned int> programs;
std::unordered_map<std::string, unsigned int> textures;
std::vector<void (*)()> imGuiCalls;

int renderableCount;

bool drawSkybox;
bool skyboxSupported;

glm::vec3 sunDirection(0, 1, 0);
glm::vec3 sunColor(0, 0, 0);
glm::vec3 ambient(0);

std::string programReverseLookup(unsigned int num){
    for (auto i : programs){
        if (i.second == num) return i.first;
    }
    return "";
}

std::string textureReverseLookup(unsigned int num){
    for (auto i : textures){
        if (i.second == num) return i.first;
    }
    return "";
}

void setSkyboxEnabled(bool enabled) {
    drawSkybox = enabled && skyboxSupported;
}

void setSunProperties(glm::vec3 position, glm::vec3 color){
    sunDirection = position;
    sunColor = color;
}

void setAmbient(float r, float g, float b){
    ambient = glm::vec3(r, g, b);
}

void setAmbient(glm::vec3 rgb){
    ambient = rgb;
}

void setMouseVisible(bool vis) {
    // Disabled-2 = normal, so if visible is true subtract 2 from mode to get normal.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED-((int)vis)*2);
}

int getRenderableCount() {
    return renderableCount;
}

int windowWidth, windowHeight;

double frameTime = 0;

double getFrameTime() {
    return frameTime;
}

std::unordered_map<std::string, GameObject> getGameObjects() {
    return gameObjects;
}

void putImGuiCall(void (*argument)()) {
    imGuiCalls.push_back(argument);
}

void registerProgram(std::string name, std::string vertex, std::string fragment, bool testDepth, bool transparencySupported, bool doubleSided) {
    unsigned int vertID = loadShader(std::move(vertex), JE_VERTEX_SHADER);
    unsigned int fragID = loadShader(std::move(fragment), JE_FRAGMENT_SHADER);
    programs.insert({name, createProgram(vertID, fragID, testDepth, transparencySupported, doubleSided)});
}

unsigned int getProgram(std::string name) {
    return programs.at(name);
}

unsigned int createTextureWithName(std::string name, std::string filePath) {
    unsigned int id = loadTexture(std::move(filePath));
#ifdef GFX_API_OPENGL41
    if (id != 0) {
        textures.insert({name, id});
    }
#else
    textures.insert({name, id});
#endif
    return id;
}

unsigned int createTexture(std::string folderPath, std::string fileName) {
    unsigned int id = loadTexture(folderPath + fileName);
#ifdef GFX_API_OPENGL41
    if (id != 0) {
        textures.insert({fileName, id});
    }
#else
    textures.insert({fileName, id});
#endif
    return id;
}

bool textureExists(std::string name) {
    return textures.count(name);
}

unsigned int getTexture(std::string name) {
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
    cpos /= glm::vec2(1, getCurrentWidth() * (1.0f / getCurrentHeight()));
    return cpos;
}

void setRawCursorPos(glm::vec2 pos) {
    glfwSetCursorPos(window, pos.x, pos.y);
}

void setCursorPos(glm::vec2 pos) {
    // inverse of getCursorPos's coordinate transformation
    pos *= glm::vec2(1, getCurrentWidth() * (1.0f / getCurrentHeight()));
    pos /= 2;
    pos -= glm::vec2(-0.5, 0.5);
    pos *= glm::vec2(getCurrentWidth(), -getCurrentHeight());
    setRawCursorPos(pos);
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

void putGameObject(std::string name, GameObject g) {
    gameObjects.insert({name, g});
}

GameObject* getGameObject(std::string name) {
    return &gameObjects.at(name);
}

int getCurrentWidth() {
    return windowWidth;
}

int getCurrentHeight() {
    return windowHeight;
}

void framebuffer_size_callback(GLFWwindow* windowInstance, int unused1, int unused2) {
    // Because of displays like Apple's Retina having multiple pixel values per pixel (or some bs like that)
    // the framebuffer is not always the window size. We need to make sure the real window size is saved.
    glfwGetWindowSize(windowInstance, &windowWidth, &windowHeight);
    resizeViewport();
}

void init(const char* windowName, int width, int height, JEGraphicsSettings graphicsSettings) {
    std::cout << "JoshEngine " << ENGINE_VERSION_STRING << std::endl;
    std::cout << "Starting engine init." << std::endl;

    windowWidth = width;
    windowHeight = height;

    ambient = {glm::max(graphicsSettings.clearColor[0] - 0.5f, 0.1f), glm::max(graphicsSettings.clearColor[1] - 0.5f, 0.1f), glm::max(graphicsSettings.clearColor[2] - 0.5f, 0.1f)};

    initGFX(&window, windowName, width, height, graphicsSettings);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Missing texture init
    createTextureWithName("missing", "./textures/missing_tex.png");
    if (!textures.count("missing")) {
        std::cerr << "Essential engine file missing." << std::endl;
        exit(1);
    }

    drawSkybox = graphicsSettings.skybox;
    skyboxSupported = graphicsSettings.skybox;

    if (graphicsSettings.skybox) {
        // Skybox init
        registerProgram("skybox", "./shaders/skybox_vertex.glsl", "./shaders/skybox_fragment.glsl", false, false, false);
        skybox = loadObj("./models/skybox.obj", getProgram("skybox"))[0];
        if (!skybox.enabled) {
            std::cerr << "Essential engine file missing." << std::endl;
            exit(1);
        }
        skybox.texture = loadCubemap({
                                             "./skybox/px_right.jpg",
                                             "./skybox/nx_left.jpg",
                                             "./skybox/py_up.jpg",
                                             "./skybox/ny_down.jpg",
                                             "./skybox/nz_front.jpg",
                                             "./skybox/pz_back.jpg"
        });
    }
    std::cout << "Graphics init successful!" << std::endl;

    initAudio();
    std::cout << "Audio init successful!" << std::endl;

    initDebugTools();
    std::cout << "Debug init successful!" << std::endl;
}

void deinit() {
    deinitGFX();
}

float fov;

void changeFOV(float n) {
    fov = n;
}

Transform* cameraAccess() {
    return &camera;
}

auto compareLambda = [](std::pair<double, Renderable> left, std::pair<double, Renderable> right){return left.first < right.first;};

void mainLoop() {
    camera = Transform(glm::vec3(0, 0, 5), glm::vec3(180, 0, 0), glm::vec3(1));
    // Initial Field of View
    fov = 78.0f;

    double currentTime = glfwGetTime();
    double lastTime = currentTime;
    double lastFrameCheck = glfwGetTime();
    double frameDrawStart;
    while (glfwWindowShouldClose(window) == 0) {
        currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

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

        for (auto & onUpdateFunction : onUpdate) {
            onUpdateFunction(deltaTime);
        }

        for (auto & g : gameObjects) {
            for (auto & gameObjectFunction : g.second.onUpdate) {
                gameObjectFunction(deltaTime, &g.second);
            }
        }

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

        updateListener(camera.position, glm::vec3(0), camera.direction(), up);

        bool doFrameTimeCheck = currentTime - lastFrameCheck > 0.1;
        if (doFrameTimeCheck) {
            lastFrameCheck = glfwGetTime();
            frameDrawStart = glfwGetTime()*1000;
        }

        std::vector<Renderable> renderables;
        std::priority_queue<std::pair<double, Renderable>, std::deque<std::pair<double, Renderable>>, decltype(compareLambda)> individualSortRenderables;

        if (drawSkybox) {
            skybox.setMatrices(camera.getTranslateMatrix(), glm::identity<mat4>(), glm::identity<mat4>());
            renderables.push_back(skybox);
        }

        for (auto item : gameObjects) {
            for (auto renderable : item.second.renderables) {
                if (renderable.enabled) {
                    renderable.setMatrices(item.second.transform.getTranslateMatrix(), item.second.transform.getRotateMatrix(), item.second.transform.getScaleMatrix());
                    if (renderable.manualDepthSort)
                        individualSortRenderables.push(std::make_pair(glm::distance(camera.position, item.second.transform.position), renderable));
                    else
                        renderables.push_back(renderable);
                }
            }
        }

        int a = individualSortRenderables.size();
        for (int i = 0; i < a; i++){
            std::pair<double, Renderable> r = individualSortRenderables.top();
            renderables.push_back(r.second);
            individualSortRenderables.pop();
        }

        renderableCount = renderables.size();

        float scaledHeight = windowHeight * (1.0f / windowWidth);
        float scaledWidth = 1.0f;
        renderFrame(camera.position,
                    camera.direction(),
                    sunDirection,
                    sunColor,
                    ambient,
                    cameraMatrix,
                    glm::ortho(-scaledWidth,scaledWidth,-scaledHeight,scaledHeight,0.0f,1.0f),
                    glm::perspective(glm::radians(fov), (float) windowWidth / (float) windowHeight, 0.01f, 500.0f),
                    renderables,
                    imGuiCalls
        );

        if (doFrameTimeCheck)
            frameTime = glfwGetTime()*1000 - frameDrawStart;

        glfwPollEvents();
    }
}