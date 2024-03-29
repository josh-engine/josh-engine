#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uvCoord;
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
                                  vec3 ambience;
};

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = (_3dProj * viewMatrix * model) * vec4(position, 1.0);
    fragColor = color;
}