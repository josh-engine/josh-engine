// JE_TRANSLATE vertex
#version 420

layout(location = 0) in vec3 pos_in;
layout(location = 1) in vec2 uv_in;
layout(location = 2) in vec3 norm_in;

layout(set = 0, binding = 0) uniform float _ts1;
layout(set = 1, binding = 0) uniform float _ts2;

layout(location = 0) out vec3 pos_out;
layout(location = 1) out vec2 uv_out;
layout(location = 2) out vec3 norm_out;

void main() {
    pos_out = pos_in - vec3(_ts1, _ts2, 0.0);
    uv_out = uv_in;
    norm_out = norm_in;
    gl_Position = vec4(pos_out, 1);
}