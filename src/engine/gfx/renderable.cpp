//
// Created by Ember Lee on 3/27/24.
//


#include "../engineconfig.h"
#ifdef GFX_API_VK
#include "vk/gfx_vk.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO((Renderable*)r);
}

#endif