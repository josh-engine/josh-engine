//
// Created by Ethan Lee on 3/9/24.
//

#ifndef JOSHENGINE3_1_ENGINE_H
#define JOSHENGINE3_1_ENGINE_H
#include "gfx/enginegfx.h"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>


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

    glm::mat4 getObjectMatrix(){
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        glm::vec3 radianRotation = glm::radians(rotation);
        auto rotationMatrix = glm::identity<mat4>();
        //TODO: There HAS to be a better way to do this.
        rotationMatrix = glm::rotate(rotationMatrix, radianRotation.x, vec3(1, 0, 0));
        rotationMatrix = glm::rotate(rotationMatrix, radianRotation.y, vec3(0, 1, 0));
        rotationMatrix = glm::rotate(rotationMatrix, radianRotation.z, vec3(0, 0, 1));
        return translationMatrix * rotationMatrix * scaleMatrix;
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
void registerOnUpdate(void (*function)(GLFWwindow** window, double dt));
void putGameObject(std::string name, GameObject g);
GameObject* getGameObject(std::string name);
void changeFOV(float n);
int getCurrentWidth();
int getCurrentHeight();
Transform* cameraAccess();

#endif //JOSHENGINE3_1_ENGINE_H
