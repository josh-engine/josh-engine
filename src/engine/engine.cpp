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
#include "gfx/modelutil.h"
#include "enginedebug.h"

GLFWwindow* window;
std::vector<void (*)(double dt)> onUpdate;
std::vector<void (*)(int key, bool pressed, double dt)> onKey;
std::map<std::string, GameObject> gameObjects;
Transform camera;
bool keys[GLFW_KEY_LAST];
Renderable skybox;
std::map<std::string, unsigned int> programs;
std::map<std::string, unsigned int> textures;
std::vector<void (*)()> imGuiCalls;

int renderableCount;

glm::vec3 sunDirection(0, 1, 0);
glm::vec3 sunColor(0, 0, 0);
glm::vec3 ambient(glm::max(CLEAR_RED - 0.5f, 0.1f), glm::max(CLEAR_GREEN - 0.5f, 0.1f), glm::max(CLEAR_BLUE - 0.5f, 0.1f));

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

std::map<std::string, GameObject> getGameObjects() {
    return gameObjects;
}

void putImGuiCall(void (*argument)()) {
    imGuiCalls.push_back(argument);
}

void registerProgram(std::string name, std::string vertex, std::string fragment, bool testDepth) {
    unsigned int vertID = loadShader(std::move(vertex), JE_VERTEX_SHADER);
    unsigned int fragID = loadShader(std::move(fragment), JE_FRAGMENT_SHADER);
    programs.insert({name, createProgram(vertID, fragID, testDepth)});
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

glm::vec2 getCursorPos() {
    double xpos, ypos;
    glfwGetCursorPos(window, (&xpos), (&ypos));
    return {xpos, ypos};
}

void setCursorPos(glm::vec2 pos) {
    glfwSetCursorPos(window, pos.x, pos.y);
}

void registerOnUpdate(void (*function)(double dt)) {
    onUpdate.push_back(function);
}

void registerOnKey(void (*function)(int key, bool pressed, double dt)) {
    onKey.push_back(function);
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

void init() {
    std::cout << "JoshEngine " << ENGINE_VERSION_STRING << std::endl;
    std::cout << "Starting engine init." << std::endl;
    windowWidth = WINDOW_WIDTH;
    windowHeight = WINDOW_HEIGHT;
    initGFX(&window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Missing texture init
    createTextureWithName("missing", "./textures/missing_tex.png");
    if (!textures.count("missing")) {
        std::cerr << "Essential engine file missing." << std::endl;
        exit(1);
    }

#ifdef DO_SKYBOX
    // Skybox init
    registerProgram("skybox", "./shaders/skybox_vertex.glsl", "./shaders/skybox_fragment.glsl", false);
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
#endif //DO_SKYBOX
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

        for (int k = 0; k < GLFW_KEY_LAST; k++) {
            bool current = glfwGetKey(window, k) == GLFW_PRESS;
            if (keys[k] != current) {
                keys[k] = current;
                for (auto & onKeyFunction : onKey) {
                    onKeyFunction(k, current, deltaTime);
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

        glm::vec3 direction(
                cos(glm::radians(camera.rotation.y)) * sin(glm::radians(camera.rotation.x)),
                sin(glm::radians(camera.rotation.y)),
                cos(glm::radians(camera.rotation.y)) * cos(glm::radians(camera.rotation.x))
        );
        // Right vector
        glm::vec3 right = glm::vec3(
                sin(glm::radians(camera.rotation.x - 90)),
                0,
                cos(glm::radians(camera.rotation.x - 90))
        );
        glm::vec3 up = glm::cross( right, direction );

        // Camera matrix
        glm::mat4 cameraMatrix = glm::lookAt(
                camera.position, // camera is at its position
                camera.position+direction, // looks in look direction
                up  // up vector
        );

        updateListener(camera.position, glm::vec3(0), direction, up);

        bool doFrameTimeCheck = currentTime - lastFrameCheck > 1;
        if (doFrameTimeCheck) {
            lastFrameCheck = glfwGetTime();
            frameDrawStart = glfwGetTime()*1000;
        }


        std::vector<Renderable> renderables;

#ifdef DO_SKYBOX
        skybox.setMatrices(camera.getTranslateMatrix(), glm::identity<mat4>(), glm::identity<mat4>());
        renderables.push_back(skybox);
#endif //DO_SKYBOX

        for (auto item : gameObjects) {
            for (auto renderable : item.second.renderables) {
                if (renderable.enabled) {
                    renderable.setMatrices(item.second.transform.getTranslateMatrix(), item.second.transform.getRotateMatrix(), item.second.transform.getScaleMatrix());
                    renderables.push_back(renderable);
                }
            }
        }

        renderableCount = renderables.size();

        float scaledHeight = windowHeight * (1.0f / windowWidth);
        float scaledWidth = 1.0f;
        glm::mat4 _2dProj;
#ifdef GFX_API_OPENGL41
        _2dProj = glm::ortho(-scaledWidth,scaledWidth,-scaledHeight,scaledHeight,0.0f,1.0f);
#elif defined(GFX_API_VK) // why the heck is elifdef a C++23 thing? i would have expected that to exist way earlier...
        _2dProj = glm::ortho(-scaledWidth,scaledWidth,scaledHeight,-scaledHeight,0.0f,1.0f);
#endif
        renderFrame(camera.position, direction, sunDirection, sunColor, ambient, cameraMatrix, _2dProj, glm::perspective(glm::radians(fov), (float) windowWidth / (float) windowHeight, 0.01f, 500.0f), renderables, imGuiCalls);

        if (doFrameTimeCheck)
            frameTime = glfwGetTime()*1000 - frameDrawStart;

        glfwPollEvents();
    }
}