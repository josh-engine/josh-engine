// JE_TRANSLATE
#version 420

layout (location = 0) in vec3 vpos;

layout (location = 0) out vec3 coords;

layout (binding = 0) uniform JE_TRANSLATE {
    mat4 matrices[4];
    vec3 camera[2];
    vec3 ambience;
};

void main()
{
    coords = vpos;
    gl_Position = matrices[0] * vec4(vpos, 1.0);
}