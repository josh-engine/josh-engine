//
// Created by Ember Lee on 3/9/24.
//
#include "gfx/enginegfx.h"
#include "sound/engineaudio.h"
#include "engineconfig.h"
#include "engine.h"
#include <iostream>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

GLFWwindow* window;
std::vector<void (*)(GLFWwindow** window, double dt)> onUpdate;
std::unordered_map<std::string, GameObject> gameObjects;
Transform camera;

int windowWidth, windowHeight;

void registerOnUpdate(void (*function)(GLFWwindow** window, double dt)){
    onUpdate.push_back(function);
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
    std::cout << "Starting engine init." << std::endl;
    windowWidth = WINDOW_WIDTH;
    windowHeight = WINDOW_HEIGHT;
    initGFX(&window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    std::cout << "Graphics init successful!" << std::endl;

    initAudio();
}
float fov;

void changeFOV(float n){
    fov = n;
}

Transform* cameraAccess() {
    return &camera;
}

void mainLoop(){
    camera = Transform(glm::vec3(0, 0, 5), glm::vec3(180, 0, 0));
    // Initial Field of View
    fov = 78.0f;

    double currentTime = glfwGetTime();
    double lastTime = currentTime;
    while (glfwWindowShouldClose(window) == 0) {
        currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        for (auto & i : onUpdate){
            i(&window, deltaTime);
        }

        for (auto & g : gameObjects){
            for (auto & i : g.second.onUpdate){
                i(deltaTime, &g.second);
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

        std::vector<Renderable> renderables;

        for (auto item : gameObjects){
            for (auto renderable : item.second.renderables) {
                if (renderable.enabled) {
                    renderables.push_back(renderable.addMatrices(item.second.transform.getTranslateMatrix(), item.second.transform.getRotateMatrix(), item.second.transform.getScaleMatrix()));
                }
            }
        }

        renderFrame(&window, cameraMatrix, camera.position, fov, renderables, windowWidth, windowHeight);
        glfwPollEvents();
    }
}