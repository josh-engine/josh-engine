// JE_TRANSLATE
#version 420

// Interpolated values from the vertex shaders
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec3 vcol;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vnorm;

// Ouput data
layout(location = 0) out vec4 color;

// Values that stay constant for the whole mesh.
layout(binding = 1) uniform sampler2D textureSampler;

void main() {
    // Output color = color of the texture at the specified UV
    color = vec4(texture(textureSampler, uv).rgb*vcol, 0.5);
}