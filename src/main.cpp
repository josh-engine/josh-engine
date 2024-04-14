#include "engine/engine.h"
#include "test.h"

int main() {
    init();
    setupTest();
    mainLoop();
    deinit();
    return 0;
}