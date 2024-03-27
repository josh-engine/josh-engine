// JE_TRANSLATE
#version 420

// Interpolated values from the vertex shaders
layout(location = 0) in vec3 vcol;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 vnorm;
layout(location = 3) in vec3 vpos;

layout(location = 0) out vec3 color;

layout(binding = 2) uniform sampler2D textureSampler;

void main(){
    vec3 normalDirection = normalize(vnorm);
    vec3 lightDirection = normalize(vec3(-1, 1, 0));
    float attenuation = 1.0;

    // default: unlit
    color = normalDirection - vec3(0.3);

    // low priority: diffuse illumination
    if (attenuation * max(0.0, dot(normalDirection, lightDirection)) >= 0.5)
    {
        color = normalDirection;
    }
}