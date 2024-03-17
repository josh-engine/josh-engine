#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 vcol;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D textureSampler;
//uniform float alpha;

void main(){
    // Output color = color of the texture at the specified UV
    color = texture(textureSampler, UV).rgb*vcol;
}