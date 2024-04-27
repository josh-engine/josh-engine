// JE_TRANSLATE
#version 420

layout (location = 0) in vec3 coords;

layout (location = 0) out vec4 color;

layout (binding = 1) uniform samplerCube skybox;

void main()
{
    color = texture(skybox, coords).rgba;
}