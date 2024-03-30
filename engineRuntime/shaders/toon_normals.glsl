// JE_TRANSLATE
#version 420

layout(location = 3) in vec3 vnorm;

layout(location = 0) out vec3 color;

void main() {
    vec3 normalDirection = normalize(vnorm);
    vec3 lightDirection = normalize(vec3(-1, 1, 0));
    float attenuation = 1.0;

    vec3 vnormCol = (normalDirection + vec3(1.0))/2.0;

    // default: unlit
    color = vnormCol - vec3(0.3);

    // low priority: diffuse illumination
    if (attenuation * max(0.0, dot(normalDirection, lightDirection)) >= 0.5)
    {
        color = vnormCol;
    }
}