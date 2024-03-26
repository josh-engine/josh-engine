//
// Created by Ember Lee on 3/9/24.
//
#include "engineconfig.h"
#include "gfx/opengl/gfx_gl33.h"
#include "gfx/vk/gfx_vk.h"
#include "sound/engineaudio.h"
#include "engine.h"
#include <iostream>
#include <unordered_map>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gfx/modelutil.h"
#include "enginedebug.h"

GLFWwindow* window;
std::vector<void (*)(double dt)> onUpdate;
std::vector<void (*)(int key, bool pressed, double dt)> onKey;
std::map<std::string, GameObject> gameObjects;
Transform camera;
bool keys[GLFW_KEY_LAST];
Renderable skybox;
std::map<std::string, GLuint> programs;
std::map<std::string, GLuint> textures;
std::vector<void (*)()> imGuiCalls;

int renderableCount;

void setMouseVisible(bool vis){
    // Disabled-2 = normal, so if visible is true subtract 2 from mode to get normal.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED-((int)vis)*2);
}

int getRenderableCount(){
    return renderableCount;
}

int windowWidth, windowHeight;

double frameTime = 0;

double getFrameTime(){
    return frameTime;
}

std::map<std::string, GameObject> getGameObjects(){
    return gameObjects;
}

void putImGuiCall(void (*argument)()) {
    imGuiCalls.push_back(argument);
}

void registerProgram(std::string name, std::string vertex, std::string fragment) {
    GLuint vertID = loadShader(std::move(vertex), GL_VERTEX_SHADER);
    GLuint fragID = loadShader(std::move(fragment), GL_FRAGMENT_SHADER);
    programs.insert({name, createProgram(vertID, fragID)});
}

GLuint getProgram(std::string name){
    return programs.at(name);
}

GLuint createTextureWithName(std::string name, std::string filePath){
    GLuint id = loadTexture(std::move(filePath));
    if (id != 0){
        textures.insert({name, id});
    }
    return id;
}

GLuint createTexture(std::string folderPath, std::string fileName){
    GLuint id = loadTexture(folderPath + fileName);
    if (id != 0){
        textures.insert({fileName, id});
    }
    return id;
}

bool textureExists(std::string name){
    return textures.count(name);
}

GLuint getTexture(std::string name){
    if (!textureExists(name)) {
        std::cerr << "Texture \"" + name + "\" not found, defaulting to missing_tex.png" << std::endl;
        return textures.at("missing");
    }
    return textures.at(name);
}

GLFWwindow** getWindow(){
    return &window;
}

bool isKeyDown(int key){
    return keys[key];
}

glm::vec2 getCursorPos(){
    double xpos, ypos;
    glfwGetCursorPos(window, (&xpos), (&ypos));
    return {xpos, ypos};
}

void setCursorPos(glm::vec2 pos){
    glfwSetCursorPos(window, pos.x, pos.y);
}

void registerOnUpdate(void (*function)(double dt)){
    onUpdate.push_back(function);
}

void registerOnKey(void (*function)(int key, bool pressed, double dt)){
    onKey.push_back(function);
}

void putGameObject(std::string name, GameObject g){
    gameObjects.insert({name, g});
}

GameObject* getGameObject(std::string name){
    return &gameObjects.at(name);
}

int getCurrentWidth() {
    return windowWidth;
}

int getCurrentHeight() {
    return windowHeight;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    windowWidth = width/2;
    windowHeight = height/2;
    glViewport(0, 0, width, height);
}

void init(){
    std::cout << "JoshEngine " << ENGINE_VERSION_STRING << std::endl;
    std::cout << "Starting engine init." << std::endl;
    windowWidth = WINDOW_WIDTH;
    windowHeight = WINDOW_HEIGHT;
    initGFX(&window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Missing texture init
    createTextureWithName("missing", "./textures/missing_tex.png");
    if (!textures.count("missing")){
        std::cerr << "Essential engine file missing." << std::endl;
        exit(1);
    }
#ifdef DO_SKYBOX
    // Skybox init
    registerProgram("skybox", "./shaders/skybox_vertex.glsl", "./shaders/skybox_fragment.glsl");
    skybox = loadObj("./models/skybox.obj", getProgram("skybox"))[0];
    if (!skybox.enabled){
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
    skybox.testDepth = false;
#endif //DO_SKYBOX
    std::cout << "Graphics init successful!" << std::endl;

    initAudio();
    std::cout << "Audio init successful!" << std::endl;

    initDebugTools();
    std::cout << "Debug init successful!" << std::endl;
}

void deinit(){
    deinitGFX(&window);
}

float fov;

void changeFOV(float n){
    fov = n;
}

Transform* cameraAccess() {
    return &camera;
}

void mainLoop(){
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

        for (int k = 0; k < GLFW_KEY_LAST; k++){
            bool current = glfwGetKey(window, k) == GLFW_PRESS;
            if (keys[k] != current) {
                keys[k] = current;
                for (auto & onKeyFunction : onKey){
                    onKeyFunction(k, current, deltaTime);
                }
            }
        }

        for (auto & onUpdateFunction : onUpdate){
            onUpdateFunction(deltaTime);
        }

        for (auto & g : gameObjects){
            for (auto & gameObjectFunction : g.second.onUpdate){
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
        if (doFrameTimeCheck){
            lastFrameCheck = glfwGetTime();
            frameDrawStart = glfwGetTime()*1000;
        }


        std::vector<Renderable> renderables;
#ifdef DO_SKYBOX
        skybox.setMatrices(camera.getTranslateMatrix(), glm::identity<mat4>(), glm::identity<mat4>());
        renderables.push_back(skybox);
#endif //DO_SKYBOX
        for (auto item : gameObjects){
            for (auto renderable : item.second.renderables) {
                if (renderable.enabled) {
                    renderable.setMatrices(item.second.transform.getTranslateMatrix(), item.second.transform.getRotateMatrix(), item.second.transform.getScaleMatrix());
                    renderables.push_back(renderable);
                }
            }
        }

        renderableCount = renderables.size();

        renderFrame(&window, cameraMatrix, camera.position, direction, fov, renderables, windowWidth, windowHeight, imGuiCalls);

        if (doFrameTimeCheck)
            frameTime = glfwGetTime()*1000 - frameDrawStart;


        glfwPollEvents();
    }
}