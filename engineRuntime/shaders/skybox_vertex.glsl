#version 330
layout (location = 0) in vec3 vpos;

out vec3 coords;

uniform mat4 MVP;

void main()
{
    coords = vpos;
    gl_Position = MVP * vec4(vpos, 1.0);
}