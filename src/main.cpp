#include "engine/engine.h"
#include "engine/sound/engineaudio.h"
#include "engine/gfx/modelutil.h"
#include "engine/enginedebug.h"

bool mouseLocked = true;
bool pressed = false;

void cameraFly(double dt) {
    float speed = 3.0f; // 3 units / second
    float mouseSpeed = 10.0f;

    Transform* camera = cameraAccess();

    if (mouseLocked) {
        glm::vec2 cursor = getCursorPos();
        setCursorPos({static_cast<float>(getCurrentWidth())/2.0f, static_cast<float>(getCurrentHeight())/2.0f});
        camera->rotation.x += mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentWidth()) /2.0f - cursor.x);
        camera->rotation.y += mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentHeight())/2.0f - cursor.y);
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
    if (isKeyDown(GLFW_KEY_W)) {
        camera->position += direction * glm::vec3(static_cast<float>(dt) * speed);
    }
    // Move backward
    if (isKeyDown(GLFW_KEY_S)) {
        camera->position -= direction * glm::vec3(static_cast<float>(dt) * speed);
    }
    // Strafe right
    if (isKeyDown(GLFW_KEY_D)) {
        camera->position += right * glm::vec3(static_cast<float>(dt) * speed);
    }
    // Strafe left
    if (isKeyDown(GLFW_KEY_A)) {
        camera->position -= right * glm::vec3(static_cast<float>(dt) * speed);
    }

    if (isKeyDown(GLFW_KEY_SPACE)) {
        camera->position += glm::vec3(0, static_cast<float>(dt) * speed, 0);
    }
    if (isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        camera->position -= glm::vec3(0, static_cast<float>(dt) * speed, 0);
    }
}

void lockUnlock(int key, bool wasKeyPressed, double dt) {
    if (key == GLFW_KEY_ESCAPE) {
        if (wasKeyPressed && !pressed) {
            mouseLocked = !mouseLocked;
            pressed = true;
        } else if (!wasKeyPressed) {
            pressed = false;
        }
        // If locked, set mouse to invisible
        setMouseVisible(!mouseLocked);
    }
}

void updateTriangle(double deltaTime, GameObject* self) {
    if (self->transform.position.y > 2) {
        self->transform.position.y = 0;
    }
    self->transform.position += vec3(0, 1*deltaTime, 0);
}

void updateBunny(double deltaTime, GameObject* self) {
    if (self->transform.rotation.y > 360) {
        self->transform.rotation.y -= 360;
    }
    self->transform.rotation += vec3(0, 40*deltaTime, 0);
}

void initTriangle(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0, 0, -2));
    selfObject->renderables.push_back(createQuad(getProgram("basicTexture"), getTexture("uv_tex.png"), true));
    selfObject->onUpdate.push_back(&updateTriangle);
}

void initTriangle2(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0, 0, 1));
    selfObject->renderables.push_back(createQuad(getProgram("basicTexture"), getTexture("uv_tex.png"), true));
    selfObject->onUpdate.push_back(&updateTriangle);
}

void initBunny(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getProgram("toonNorm"));
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&updateBunny);
}

void initBunny2(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(-2, 0, 0));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getProgram("bnphColor"));
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&updateBunny);
}

void initBunny3(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0, 0, -4));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getProgram("bnphColor"));
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&updateBunny);
}

void initCube(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(3, 1, 0));
    std::vector<Renderable> modelRenderables = loadObj("./models/cube-tex.obj", getProgram("bnphTexture"));
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&updateBunny);
}

void initTriangle3(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(-0.5, -0.25, -1), glm::vec3(0), glm::vec3(0.25));
    selfObject->renderables.push_back(createQuad(getProgram("ui"), getTexture("missing")));
}

int main() {
    init();

    //     sun-ish direction from skybox, slightly warm white
    setSunProperties(glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0.85));

    setMouseVisible(false); // Mouse starts locked

    Sound st2 = Sound(glm::vec3(0), glm::vec3(0), "./sounds/explosion-mono.ogg", true, 3, 0.1, 2, 0.25);
    st2.play();

    registerOnUpdate(&cameraFly);
    registerOnKey(&lockUnlock);

    registerFunctionToDebug("updateBunny",    reinterpret_cast<void*>(&updateBunny));
    registerFunctionToDebug("updateTriangle", reinterpret_cast<void*>(&updateTriangle));

    registerProgram("toonNorm", "./shaders/vertex3d.glsl", "./shaders/toon_normals.glsl", true, false, false);
    registerProgram("bnphColor", "./shaders/vertex3d.glsl", "./shaders/blinn-phong_color.glsl", true, false, false);
    registerProgram("ui", "./shaders/vertex2d.glsl", "./shaders/frag_tex.glsl", true, false, false);
    registerProgram("bnphTexture", "./shaders/vertex3d.glsl", "./shaders/blinn-phong_textured.glsl", true, false, false);
    registerProgram("basicTexture", "./shaders/vertex3d.glsl", "./shaders/frag_tex_transparent.glsl", true, true, true);

    createTexture("./textures/", "uv_tex.png");
    createTextureWithName("cube_texture", "./textures/cubetex.png");

    putGameObject("triangle_test", GameObject(&initTriangle));
    putGameObject("triangle_test_2", GameObject(&initTriangle2));
    putGameObject("bunny", GameObject(&initBunny));
    putGameObject("bunny2", GameObject(&initBunny2));
    putGameObject("bunny3", GameObject(&initBunny3));
    putGameObject("cube", GameObject(&initCube));
    putGameObject("ui_item", GameObject(&initTriangle3));

    mainLoop();

    deinit();
    return 0;
}