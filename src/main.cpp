#include <iostream>
#include "engine/engineconfig.h"
#include "engine/engine.h"
#include "engine/gfx/modelutil.h"
#include "GLFW/glfw3.h"

bool mouseLocked = true;
bool pressed = false;

void cameraFly(GLFWwindow** window, double dt){

    float speed = 3.0f; // 3 units / second
    float mouseSpeed = 10.0f;

    Transform* camera = cameraAccess();

    if (mouseLocked){
        double xpos, ypos;
        glfwGetCursorPos(*window, (&xpos), (&ypos));
        glfwSetCursorPos(*window, getCurrentWidth()/2.0f, getCurrentHeight()/2.0f);
        camera->rotation.x += mouseSpeed * dt * float(getCurrentWidth() /2.0f - xpos );
        camera->rotation.y += mouseSpeed * dt * float(getCurrentHeight()/2.0f - ypos );
        camera->rotation.y = clamp(camera->rotation.y, -70.0f, 70.0f);
    }

    glm::vec3 direction(
            cos(glm::radians(camera->rotation.y)) * sin(glm::radians(camera->rotation.x)),
            sin(glm::radians(camera->rotation.y)),
            cos(glm::radians(camera->rotation.y)) * cos(glm::radians(camera->rotation.x))
    );

    // Right vector
    glm::vec3 right = glm::vec3(
            sin(glm::radians(camera->rotation.x - 90)),
            0,
            cos(glm::radians(camera->rotation.x - 90))
    );

    // Move forward
    if (glfwGetKey(*window, GLFW_KEY_W ) == GLFW_PRESS){
        camera->position += direction * glm::vec3(dt * speed);
    }
    // Move backward
    if (glfwGetKey(*window, GLFW_KEY_S ) == GLFW_PRESS){
        camera->position -= direction * glm::vec3(dt * speed);
    }
    // Strafe right
    if (glfwGetKey(*window, GLFW_KEY_D ) == GLFW_PRESS){
        camera->position += right * glm::vec3(dt * speed);
    }
    // Strafe left
    if (glfwGetKey(*window, GLFW_KEY_A ) == GLFW_PRESS){
        camera->position -= right * glm::vec3(dt * speed);
    }

    if (glfwGetKey(*window, GLFW_KEY_SPACE) == GLFW_PRESS){
        camera->position += glm::vec3(0, dt * speed, 0);
    }
    if (glfwGetKey(*window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        camera->position -= glm::vec3(0, dt * speed, 0);
    }
}

void lockUnlock(GLFWwindow** window, double dt){
    if (glfwGetKey(*window, GLFW_KEY_ESCAPE ) == GLFW_PRESS && !pressed){
        mouseLocked = !mouseLocked;
        pressed = true;
    } else if (glfwGetKey(*window, GLFW_KEY_ESCAPE ) == GLFW_RELEASE) {
        pressed = false;
    }
}

void updateTriangle(double deltaTime, GameObject* self){
    self->transform.position += vec3(0, 1*deltaTime, 0);
}

void updateTriangle2(double deltaTime, GameObject* self){
    self->transform.rotation += vec3(10*deltaTime, 20*deltaTime, 30*deltaTime);
}

Renderable quad;

void initTriangle(GameObject* selfObject){
    selfObject->transform = Transform(glm::vec3(0, 0, -2));
    selfObject->renderables.push_back(quad);
    selfObject->onUpdate.push_back(&updateTriangle);
}

void initTriangle2(GameObject* selfObject){
    selfObject->transform = Transform(glm::vec3(0));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getProgram("basicColor"));
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    //selfObject->renderables.push_back(quad);
    selfObject->onUpdate.push_back(&updateTriangle2);
}

void initTriangle3(GameObject* selfObject){
    selfObject->transform = Transform(glm::vec3(-0.5, -0.25, -1), glm::vec3(0), glm::vec3(0.25));
    Renderable r = quad;
    r.is3d = false;
    r.texture = getTexture("thisShouldBeMissing");
    selfObject->renderables.push_back(r);
}

int main() {
    init();

    registerOnUpdate(&cameraFly);
    registerOnUpdate(&lockUnlock);

    registerProgram("basicColor", "./shaders/vertex.glsl", "./shaders/frag_col.glsl");
    registerProgram("basicTexture", "./shaders/vertex.glsl", "./shaders/frag_tex.glsl");
    createTexture("./textures/", "uv_tex.png");

    quad = createQuad(true, getProgram("basicTexture"), getTexture("uv_tex.png"));

    putGameObject("triangle_test", GameObject(&initTriangle));
    putGameObject("triangle_test_2", GameObject(&initTriangle2));
    putGameObject("triangle_test_3", GameObject(&initTriangle3));

    mainLoop();
    return 0;
}