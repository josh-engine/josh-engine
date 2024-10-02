//
// Created by Ember Lee on 4/14/24.
//

#include "test.h"
#include "engine/engine.h"
#include "engine/ui/je/uiutil.h"
#include "engine/sound/audioutil.h"
#include "engine/gfx/modelutil.h"
#include "engine/debug/debugutil.h"

bool mouseLocked = false;
bool pressed = false;

const float speed = 3.0f; // 3 units / second
const float mouseSpeed = 10.0f;

void cameraFly(double dt) {
    if (mouseLocked) {
        Transform* camera = cameraAccess();

        glm::vec2 cursor = getRawCursorPos();
        setRawCursorPos({static_cast<float>(getCurrentWidth()) / 2.0f, static_cast<float>(getCurrentHeight()) / 2.0f});
        camera->rotation.x +=
                mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentWidth()) / 2.0f - cursor.x);
        camera->rotation.y +=
                mouseSpeed * static_cast<float>(dt) * (static_cast<float>(getCurrentHeight()) / 2.0f - cursor.y);
        camera->rotation.y = clamp(camera->rotation.y, -70.0f, 70.0f);


        // Right vector
        glm::vec3 right = glm::vec3(
                sin(glm::radians(camera->rotation.x - 90)),
                0,
                cos(glm::radians(camera->rotation.x - 90))
        );


        // Move forward
        if (isKeyDown(GLFW_KEY_W)) {
            camera->position += camera->direction() * glm::vec3(static_cast<float>(dt) * speed);
        }
        // Move backward
        if (isKeyDown(GLFW_KEY_S)) {
            camera->position -= camera->direction() * glm::vec3(static_cast<float>(dt) * speed);
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

void mouseClick(int button, bool wasButtonPressed, double dt) {
    glm::vec2 cursorPos = getCursorPos();
    GameObject& ref = getGameObject("ui_item");

    if (cursorPos.x <= ref.transform.position.x + ref.transform.scale.x &&
        cursorPos.x >= ref.transform.position.x - ref.transform.scale.x &&
        cursorPos.y <= ref.transform.position.y + ref.transform.scale.y &&
        cursorPos.y >= ref.transform.position.y - ref.transform.scale.y) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && wasButtonPressed) {
            ref.transform.position += vec3(0, 0.1, 0);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT && wasButtonPressed) {
            ref.transform.position -= vec3(0, 0.1, 0);
        }
    }
}

void move(double deltaTime, GameObject* self) {
    if (self->transform.position.y > 2) {
        self->transform.position.y = 0;
    }
    self->transform.position += vec3(0, 1*deltaTime, 0);
}

void spin(double deltaTime, GameObject* self) {
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
    selfObject->transform = Transform(glm::vec3(-0.5, -0.25, 0), glm::vec3(0), glm::vec3(0.25));
    selfObject->renderables.push_back(createQuad(getShader("ui"), {0, getTexture("missing")}));
}

MusicTrack test{};

void btnTest() {

}

void setupTest() {
    //     sun-ish direction from skybox, slightly warm white
    setSunProperties(glm::vec3(-1, 1, -1), glm::vec3(1, 1, 0.85));

    setMouseVisible(false); // Mouse starts locked
    mouseLocked = true;

    registerOnUpdate(&cameraFly);
    registerOnKey(&lockUnlock);
    registerOnMouse(&mouseClick);

    //registerFunctionToDebug("spin", reinterpret_cast<void*>(&spin));
    registerFunctionToDebug("move", reinterpret_cast<void*>(&move));


    JEShaderProgramSettings a{};
    a.testDepth = true;
    a.transparencySupported = false;
    a.doubleSided = false;
    //               This means the layout will be {Uniform, Uniform}.
    //               Everything here is redundant because Uniform Bit is zero. Just is easier to read this way.
    a.shaderInputs = JEShaderInputUniformBit | (JEShaderInputUniformBit << 1);
    a.shaderInputCount = 2;

    createShader("toonNorm", "./shaders/vertex3d.glsl", "./shaders/toon_normals.glsl", a);
    createShader("bnphColor", "./shaders/vertex3d.glsl", "./shaders/blinn-phong_color.glsl", a);

    //               This means the layout will be {Uniform, Texture}.
    a.shaderInputs = JEShaderInputUniformBit | (JEShaderInputTextureBit << 1);

    createShader("ui", "./shaders/vertex2d.glsl", "./shaders/frag_tex.glsl", a);

    //               This layout is {Uniform, Uniform, Texture}.
    a.shaderInputs = JEShaderInputUniformBit | (JEShaderInputUniformBit << 1) | (JEShaderInputTextureBit << 2);
    a.shaderInputCount = 3;

    createShader("bnphTexture", "./shaders/vertex3d.glsl", "./shaders/blinn-phong_textured.glsl", a);

    JEShaderProgramSettings b{};
    b.testDepth = true;
    b.transparencySupported = true;
    b.doubleSided = true;
    b.shaderInputs = JEShaderInputUniformBit | (JEShaderInputTextureBit << 1);
    b.shaderInputCount = 2;
    createShader("basicTexture", "./shaders/vertex3d.glsl", "./shaders/frag_tex_transparent.glsl", b);

    createTexture("manifold", "./textures/rebuildFont.bmp");

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

    //Sound st2 = Sound(glm::vec3(0), glm::vec3(0), "./sounds/explosion-mono.ogg", true, 3, 0.1, 2, 0.25);
    //st2.play();

    test.setBuffer(0, oggToBuffer("./sounds/music/hypertensile-startup.ogg"));
    test.setBuffer(1, oggToBuffer("./sounds/music/hypertensile-menu.ogg"));
    test.setBuffer(2, oggToBuffer("./sounds/music/hypertensile-connect.ogg"));
    test.setBuffer(3, oggToBuffer("./sounds/music/hypertensile-system.ogg"));
    test.setBuffer(4, oggToBuffer("./sounds/music/hypertensile-planet.ogg"));
    test.setBuffer(5, oggToBuffer("./sounds/music/hypertensile-load.ogg"));

    // Queue intro
    test.queue(0);
    test.queue(1);
    test.play();
    // Unqueue intro (menu loop plays three times, loops clean once)
    test.waitMS(5*1000);
    test.unqueue(0);

    // Queue online connection and system view select, unqueue
    test.waitMS(40*1000);
    test.queue(2);
    test.unqueue(1);
    test.queue(3);

    // Unqueue online connection
    test.waitMS(55*1000);
    test.unqueue(2);

    uiStaticButton({0.5, -0.2}, "test", "manifold", &btnTest);
}