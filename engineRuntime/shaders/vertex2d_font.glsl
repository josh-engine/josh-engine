// JE_TRANSLATE vertex
#version 420

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal;

layout(push_constant) uniform PushConstants { // JE_TRANSLATE
    mat4 model;
    mat4 character_data;
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
layout(location = 2) flat out uint character;
layout(location = 3) out vec3 fontcolor;

void main() {
    vec3 vpm = vec3(vertexPosition_modelspace.x + character_data[0][0], vertexPosition_modelspace.y*-1 + character_data[0][1], 0);
    gl_Position = (_2dProj * model) * vec4(vpm, 1);
    vec4 pos = (model * vec4(vpm, 1));
    vpos = pos.xyz;
    character = floatBitsToUint(character_data[0][2]);
    uv = vertexUV;
    fontcolor = vec3(character_data[1][0], character_data[1][1], character_data[1][2]);
}