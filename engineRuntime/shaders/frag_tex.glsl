// JE_TRANSLATE
#version 420

// Interpolated values from the vertex shaders
layout(location = 0) in vec3 vcol;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 vnorm;
layout(location = 3) in vec3 vpos;

// Ouput data
layout(location = 0) out vec3 color;

// Values that stay constant for the whole mesh.
layout(binding = 2) uniform sampler2D textureSampler;
//uniform float alpha;

void main(){
    // Output color = color of the texture at the specified UV
    color = texture(textureSampler, UV).rgb*vcol;
}