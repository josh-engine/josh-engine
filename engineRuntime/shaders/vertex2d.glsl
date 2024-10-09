// JE_TRANSLATE
#version 420

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;

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

layout(location = 0) out vec3 vpos;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 vnorm;

void main() {
    gl_Position = (_2dProj * model) * vec4(vertexPosition_modelspace,1);
    vec4 pos = (model * vec4(vertexPosition_modelspace,1));
    vpos = pos.xyz;
    uv = vertexUV;
    vnorm = mat3(transpose(inverse(model))) * vnorm;
}