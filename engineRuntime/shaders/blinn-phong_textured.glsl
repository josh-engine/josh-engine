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
    vec3 normal = normalize(vnorm);
    vec3 lightDir = vec3(1, 1, 0);
    float distance = length(lightDir);
    distance = distance * distance;
    lightDir = normalize(lightDir);

    float lambertian = max(dot(lightDir, normal), 0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {

        vec3 viewDir = normalize(-vpos);

        // this is blinn phong
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, 1);
    }

    color = texture(textureSampler, UV).rgb * (vec3(0.2) + vcol * lambertian * vec3(1.0));
}