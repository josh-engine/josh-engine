// JE_TRANSLATE
#version 420

layout(location = 1) in vec3 vcol;

layout(location = 0) out vec3 color;

void main() {
    // Output color = color of the texture at the specified UV
    color = vcol;
}