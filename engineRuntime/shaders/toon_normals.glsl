// JE_TRANSLATE
#version 420

layout(location = 3) in vec3 vnorm;

layout(location = 0) out vec3 color;

layout(binding = 0) uniform UBO { // JE_TRANSLATE
    mat4 viewMatrix;
    mat4 _2dProj;
    mat4 _3dProj;
    vec3 cameraPos;
    vec3 cameraDir;
    vec3 sunDir;
    vec3 sunColor;
    vec3 ambience;
};

void main() {
    vec3 normalDirection = normalize(vnorm);
    vec3 lightDirection = normalize(sunDir);
    float attenuation = 1.0;

    vec3 vnormCol = (normalDirection + vec3(1.0))/2.0;

    // default: unlit
    color = (vnormCol - vec3(1)) + ambience;

    float diff = attenuation * max(0.0, dot(normalDirection, lightDirection));
    // low priority: diffuse illumination
    if (diff < 0.3)
    {
        color = (vnormCol - vec3(0.7)) + ambience;
    } else if (diff < 0.5) {
        color = (vnormCol - vec3(0.5)) + ambience;
    } else if (diff < 0.7) {
        color = (vnormCol - vec3(0.3)) + ambience;
    } else {
        color = (vnormCol) + ambience;
    }
}