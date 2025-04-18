//
// Created by Ember Lee on 4/14/24.
//

#include "test.h"
#include "engine/engine.h"
#include "engine/engineconfig.h"
#include "engine/ui/je/uiutil.h"
#include "engine/sound/audioutil.h"
#include "engine/gfx/modelutil.h"
#include "engine/debug/debugutil.h"

using namespace JE;

bool mouseLocked = false;
bool pressed = false;

const float speed = 3.0f; // 3 units / second
const float mouseSpeed = 10.0f;

void cameraFly(double dt) {
    if (mouseLocked) {
        Transform* camera = cameraAccess();

        static vec2 lastCursorPos = {static_cast<float>(getCurrentWidth()) / 2.0f, static_cast<float>(getCurrentHeight()) / 2.0f};
        const vec2 cursor = getRawCursorPos();
        camera->rotation.x +=
                mouseSpeed * static_cast<float>(dt) * (lastCursorPos.x - cursor.x);
        camera->rotation.y +=
                mouseSpeed * static_cast<float>(dt) * (lastCursorPos.y - cursor.y);
        camera->rotation.y = clamp(camera->rotation.y, -70.0f, 70.0f);
        lastCursorPos = cursor;


        // Right vector
        const auto right = vec3(
                sin(glm::radians(camera->rotation.x - 90)),
                0,
                cos(glm::radians(camera->rotation.x - 90))
        );


        // Move forward
        if (isButtonDown(FORWARD)) {
            camera->position += camera->direction() * vec3(static_cast<float>(dt) * speed);
        }
        // Move backward
        if (isButtonDown(BACKWARD)) {
            camera->position -= camera->direction() * vec3(static_cast<float>(dt) * speed);
        }
        // Strafe right
        if (isButtonDown(RIGHT)) {
            camera->position += right * vec3(static_cast<float>(dt) * speed);
        }
        // Strafe left
        if (isButtonDown(LEFT)) {
            camera->position -= right * vec3(static_cast<float>(dt) * speed);
        }

        if (isButtonDown(UP)) {
            camera->position += vec3(0, static_cast<float>(dt) * speed, 0);
        }
        if (isButtonDown(DOWN)) {
            camera->position -= vec3(0, static_cast<float>(dt) * speed, 0);
        }
    }
}

void lockUnlock(const ButtonInput button, const bool wasButtonPressed) {
    if (button == UI_EXIT) {
        if (wasButtonPressed && !pressed) {
            mouseLocked = !mouseLocked;
            pressed = true;
        } else if (!wasButtonPressed) {
            pressed = false;
        }
        // If locked, set mouse to invisible
        setMouseVisible(!mouseLocked);
    }
}

void mouseClick(const int button, const bool wasButtonPressed) {
    const glm::vec2 cursorPos = getCursorPos();
    GameObject& ref = getGameObject("ui_item");

    if (cursorPos.x <= ref.transform.position.x + ref.transform.scale.x &&
        cursorPos.x >= ref.transform.position.x - ref.transform.scale.x &&
        cursorPos.y <= ref.transform.position.y + ref.transform.scale.y &&
        cursorPos.y >= ref.transform.position.y - ref.transform.scale.y) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && wasButtonPressed) {
            ref.transform.position += vec3(0, 1, 0);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT && wasButtonPressed) {
            ref.transform.position -= vec3(0, 1, 0);
        }
    }
}

void move(const double deltaTime, GameObject* self) {
    if (self->transform.position.y > 2) {
        self->transform.position.y = 0;
    }
    self->transform.position += vec3(0, 1*deltaTime, 0);
}

void spin(const double deltaTime, GameObject* self) {
    if (self->transform.rotation.y > 360) {
        self->transform.rotation.y -= 360;
    }
    self->transform.rotation += vec3(0, 40*deltaTime, 0);
}

void initTriangle(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0, 0, -2));
    selfObject->renderables.push_back(createQuad(getShader("basicTexture"), {getUBOID(), getTexture("uv_tex.png")}, true));
    selfObject->onUpdate.push_back(&move);
}

void initTriangle2(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0, 0, 1));
    selfObject->renderables.push_back(createQuad(getShader("basicTexture"), {getUBOID(), getTexture("logo.png")}, true));
    selfObject->onUpdate.push_back(&move);
}

void initBunny(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getShader("toonNorm"), {getUBOID(), getLBOID()});
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&spin);
}

void initBunny2(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(-2, 0, 0));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getShader("bnphColor"), {getUBOID(), getLBOID()});
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&spin);
}

void initBunny3(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(0, 0, -4));
    std::vector<Renderable> modelRenderables = loadObj("./models/bunny.obj", getShader("bnphColor"), {getUBOID(), getLBOID()});
    selfObject->renderables.insert(selfObject->renderables.end(), modelRenderables.begin(), modelRenderables.end());
    selfObject->onUpdate.push_back(&spin);
}

void initCube(GameObject* selfObject) {
    selfObject->transform = Transform(glm::vec3(3, 1, 0));
    std::vector<Renderable> modelRenderables = loadObj("./models/cube-tex.obj", getShader("bnphTexture"), {getUBOID(), getLBOID(), getTexture("cube_texture")});
    selfObject->renderables.push_back(modelRenderables[0]);
    selfObject->onUpdate.push_back(&spin);
}

void initUiItem(GameObject* selfObject) {
    selfObject->transform = Transform(vec3(-5, -2.5, 0), vec3(0), vec3(2.5));
    selfObject->renderables.push_back(createQuad(getShader("ui"), {getUBOID(), getTexture("missing")}, false, true));
}

void setupTest() {
    //     sun-ish direction from skybox, slightly warm white
    setSunProperties(vec3(-1, 1, -1), vec3(1, 1, 0.85));

    setMouseVisible(false); // Mouse starts locked
    mouseLocked = true;

    registerOnUpdate(&cameraFly);
    registerOnButton(&lockUnlock);
    registerOnMouse(&mouseClick);

    //registerFunctionToDebug("spin", reinterpret_cast<void*>(&spin));
    registerFunctionToDebug("move", reinterpret_cast<void*>(&move));


    ShaderProgramSettings a{};
    a.testDepth = true;
    a.transparencySupported = false;
    a.doubleSided = false;
    //               This means the layout will be {Uniform, Uniform}.
    //               Everything here is redundant because Uniform Bit is zero. Just is easier to read this way.
    a.shaderInputs = ShaderInputBit::Uniform | (ShaderInputBit::Uniform << 1);
    a.shaderInputCount = 2;

    createShader("toonNorm", "./shaders/vertex3d.glsl", "./shaders/toon_normals.glsl", a);
    createShader("bnphColor", "./shaders/vertex3d.glsl", "./shaders/blinn-phong_color.glsl", a);

    //               This means the layout will be {Uniform, Texture}.
    a.shaderInputs = ShaderInputBit::Uniform | (ShaderInputBit::Texture << 1);

    createShader("ui", "./shaders/vertex2d.glsl", "./shaders/frag_tex.glsl", a);

    //               This layout is {Uniform, Uniform, Texture}.
    a.shaderInputs = ShaderInputBit::Uniform | (ShaderInputBit::Uniform << 1) | (ShaderInputBit::Texture << 2);
    a.shaderInputCount = 3;

    a.testDepth = true;

    createShader("bnphTexture", "./shaders/vertex3d.glsl", "./shaders/blinn-phong_textured.glsl", a);

    ShaderProgramSettings b{};
    b.testDepth = true;
    b.transparencySupported = true;
    b.doubleSided = true;
    b.shaderInputs = ShaderInputBit::Uniform | (ShaderInputBit::Texture << 1);
    b.shaderInputCount = 2;
    createShader("basicTexture", "./shaders/vertex3d.glsl", "./shaders/frag_tex_transparent.glsl", b);

    UI::createFont("manifold", "./textures/manifold.bmp", {0.5, 0.7, (5.0/64.0)});

    createTexture("uv_tex.png", "./textures/uv_tex.png");
    createTexture("logo.png", "./textures/logo.png");
    createTexture("cube_texture", "./textures/cubetex.png");

    putGameObject("triangle_test", GameObject(&initTriangle));
    putGameObject("triangle_test_2", GameObject(&initTriangle2));
    putGameObject("bunny", GameObject(&initBunny));
    putGameObject("bunny2", GameObject(&initBunny2));
    putGameObject("bunny3", GameObject(&initBunny3));
    putGameObject("cube", GameObject(&initCube));
    putGameObject("ui_item", GameObject(&initUiItem));

    Audio::Sound st2 = Audio::Sound(glm::vec3(0), glm::vec3(0), "./sounds/explosion-mono.ogg", true, 3, 0.1, 2, 0.25);
    st2.play(); // this noise is so annoying now

    UI::staticButton({-10.0+UI::getTextWidth("press for segfault", "manifold"), -5.25},
                        "press for segfault",
                        "manifold",
                        nullptr,
                        {0.05, 0.05},
                        0.25,
                        {1, 1, 1},
                        false);

    UI::staticText({10.0-UI::getTextWidth(ENGINE_VERSION_STRING, "manifold"), -5.25}, ENGINE_VERSION_STRING, "manifold");

    UI::staticTextureQuad({-0.5, -5.0}, {0.25, 0.25}, "uv_tex.png");

    UI::staticColorQuad({-0.5, -4.0}, {0.25, 0.25}, {1, 0, 0, 0.8});
    UI::staticColorQuad({0.0, -4.0}, {0.25, 0.25}, {0, 1, 0, 0.5});
    UI::staticColorQuad({0.5, -4.0}, {0.25, 0.25}, {0, 0, 1, 0.2});
}