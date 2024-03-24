#version 330
out vec3 color;

in vec3 coords;

uniform samplerCube skybox;

void main()
{
    color = texture(skybox, coords).rgb;
}