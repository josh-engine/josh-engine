// JE_TRANSLATE
#version 420

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec3 vertexNormal;

layout(push_constant) uniform PushConstants { // JE_TRANSLATE
    mat4 model;
    mat4 normal;
};

layout(binding = 0) uniform UBO { // JE_TRANSLATE
    mat4 viewMatrix;
    mat4 _2dProj;
    mat4 _3dProj;
    vec3 cameraPos;
    vec3 cameraDir;
    vec3 sunDir;
    vec3 sunColor;
    vec3 ambience;
};

layout(location = 0) out vec3 vpos;
layout(location = 1) out vec3 vcol;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec3 vnorm;

void main() {
    gl_Position = (_3dProj * viewMatrix * model) * vec4(vertexPosition_modelspace,1);
    vec4 pos = (model * vec4(vertexPosition_modelspace,1));
    vec4 normalv4 = (normal * vec4(vertexNormal,1));
    vcol = vertexColor;
    vpos = pos.xyz;
    uv = vertexUV;
    vnorm = normalv4.xyz;
}