#version 330
layout (location = 0) in vec3 vpos;

out vec3 coords;

uniform mat4 matrices[4];

void main()
{
    coords = vpos;
    gl_Position = matrices[0] * vec4(vpos, 1.0);
}