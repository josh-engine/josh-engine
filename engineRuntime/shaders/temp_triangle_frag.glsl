// JE_TRANSLATE fragment
#version 420

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 vnorm;

layout(location = 0) out vec4 color;

void main() {
    color = vec4(uv, 0.0, 1.0);
}