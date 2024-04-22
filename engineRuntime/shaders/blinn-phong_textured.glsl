// JE_TRANSLATE
#version 420

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 vnorm;

layout(location = 0) out vec3 color;

layout(binding = 1) uniform sampler2D textureSampler;

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
    vec3 normal = normalize(vnorm);
    vec3 lightDir = normalize(sunDir);
    float distance = length(lightDir);
    distance = distance * distance;
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {

        vec3 viewDir = normalize(-vpos);

        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, 1);
    }

    color = texture(textureSampler, uv).rgb * (ambience + vec3(1) * lambertian * sunColor);
}