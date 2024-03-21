#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 vcol;
in vec3 vnorm;
in vec3 vpos;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D textureSampler;
//uniform float alpha;

void main(){
    vec3 normalDirection = normalize(vnorm);
    vec3 lightDirection = normalize(vec3(1, 1, 0));
    float attenuation = 1.0;

    // default: unlit
    color = normalDirection - vec3(0.3);

    // low priority: diffuse illumination
    if (attenuation * max(0.0, dot(normalDirection, lightDirection)) >= 0.5)
    {
        color = normalDirection;
    }
}