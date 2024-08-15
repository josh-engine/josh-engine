#include "engine/engine.h"
#include "game.h"

int main() {
    JEGraphicsSettings graphicsSettings{};
    graphicsSettings.vsyncEnabled = true;
    graphicsSettings.skybox = true;
    graphicsSettings.clearColor[0] = 0.75f;
    graphicsSettings.clearColor[1] = 0.75f;
    graphicsSettings.clearColor[2] = 0.8f;
    graphicsSettings.msaaSamples = 4;

    init("JoshEngine Demo", 1280, 720, graphicsSettings);
    gameSetup();
    mainLoop();
    deinit();
    return 0;
}