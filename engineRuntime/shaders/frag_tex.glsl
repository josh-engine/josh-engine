// JE_TRANSLATE
#version 420

// Interpolated values from the vertex shaders
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 vnorm;

// Ouput data
layout(location = 0) out vec3 color;

// Values that stay constant for the whole mesh.
layout(binding = 1) uniform sampler2D textureSampler;

void main() {
    // Output color = color of the texture at the specified UV
    color = texture(textureSampler, uv).rgb;
}