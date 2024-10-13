#include "engine/engine.h"
#include "test.h"

int main() {
    JE::GraphicsSettings graphicsSettings{};
    graphicsSettings.vsyncEnabled = true;
    graphicsSettings.skybox = true;
    graphicsSettings.clearColor[0] = 0.75f;
    graphicsSettings.clearColor[1] = 0.75f;
    graphicsSettings.clearColor[2] = 0.8f;
    graphicsSettings.msaaSamples = 4;

    JE::init("JoshEngine Demo", 1280, 720, graphicsSettings);
    setupTest();
    JE::mainLoop();
    JE::deinit();
    return 0;
}