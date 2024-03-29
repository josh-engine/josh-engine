//
// Created by Ember Lee on 3/9/24.
//

#ifndef JOSHENGINE_ENGINE_H
#define JOSHENGINE_ENGINE_H
#include <glm/glm.hpp>
#include <map>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "gfx/renderable.h"

using namespace glm;

class Transform {
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform(){
        position = vec3(0.0f);
        rotation = vec3(0.0f);
        scale = vec3(1.0f);
    }

    Transform(glm::vec3 pos){
        position = pos;
        rotation = vec3(0.0f);
        scale = vec3(1.0f);
    }

    Transform(glm::vec3 pos, glm::vec3 rot){
        position = pos;
        rotation = rot;
        scale = vec3(1.0f);
    }

    Transform(glm::vec3 pos, glm::vec3 rot, glm::vec3 sca){
        position = pos;
        rotation = rot;
        scale = sca;
    }

    glm::mat4 getTranslateMatrix(){
        return glm::translate(glm::mat4(1.0f), position);
    }

    glm::mat4 getRotateMatrix(){
        glm::vec3 radianRotation = glm::radians(rotation);
        auto rotationMatrix = glm::identity<mat4>();
        //TODO: There HAS to be a better way to do this.
        rotationMatrix = glm::rotate(rotationMatrix, radianRotation.x, vec3(1, 0, 0));
        rotationMatrix = glm::rotate(rotationMatrix, radianRotation.y, vec3(0, 1, 0));
        rotationMatrix = glm::rotate(rotationMatrix, radianRotation.z, vec3(0, 0, 1));
        return rotationMatrix;
    }

    glm::mat4 getScaleMatrix(){
        return glm::scale(glm::mat4(1.0f), scale);
    }
};

class GameObject {
public:
    Transform transform;
    std::vector<void (*)(double dt, GameObject* g)> onUpdate = {};
    std::vector<Renderable> renderables = {};

    explicit GameObject(void (*initFunc)(GameObject *g)){
        transform = Transform();
        initFunc(this);
    }

    GameObject(Transform t, void (*initFunc)(GameObject *g)){
        transform = t;
        initFunc(this);
    }
};

void init();
void mainLoop();
void deinit();
void registerOnUpdate(void (*function)(double dt));
void registerOnKey(void (*function)(int key, bool pressed, double dt));
void putGameObject(std::string name, GameObject g);
std::map<std::string, GameObject> getGameObjects();
void putImGuiCall(void (*argument)());
GameObject* getGameObject(std::string name);
void changeFOV(float n);
int getCurrentWidth();
int getCurrentHeight();
Transform* cameraAccess();
bool isKeyDown(int key);
GLFWwindow** getWindow();
glm::vec2 getCursorPos();
void setCursorPos(glm::vec2 pos);
unsigned int createTextureWithName(std::string name, std::string fileName);
unsigned int createTexture(std::string folderPath, std::string fileName);
unsigned int getTexture(std::string name);
bool textureExists(std::string name);
unsigned int getProgram(std::string name);
void registerProgram(std::string name, std::string vertex, std::string fragment, bool testDepth);
double getFrameTime();
int getRenderableCount();
void setMouseVisible(bool vis);

#endif //JOSHENGINE_ENGINE_H
