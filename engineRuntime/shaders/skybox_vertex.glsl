// JE_TRANSLATE
#version 420

layout (location = 0) in vec3 vpos;

layout (location = 0) out vec3 coords;

layout(push_constant) uniform PushConstants { // JE_TRANSLATE
    mat4 model;
    mat4 free_real_estate;
};

layout(set = 0, binding = 0) uniform UBO { // JE_TRANSLATE
    mat4 viewMatrix;
    mat4 _2dProj;
    mat4 _3dProj;
    vec3 cameraPos;
    vec3 cameraDir;
    vec2 screenSize;
};

void main()
{
    coords = vpos;
    gl_Position = _3dProj * viewMatrix * model * vec4(vpos, 1.0);
}