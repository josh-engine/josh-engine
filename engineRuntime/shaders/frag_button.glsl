// JE_TRANSLATE fragment
#version 420

// Interpolated values from the vertex shaders
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 vnorm;

layout(push_constant) uniform PushConstants { // JE_TRANSLATE
    mat4 model;
    mat4 free_real_estate;
};

// Ouput data
layout(location = 0) out vec4 color;

void main() {
    // Output color = color of the texture at the specified UV
    color = vec4(free_real_estate[0][0], free_real_estate[0][1], free_real_estate[0][2], free_real_estate[0][3]);
}