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
    vec3 lightDir = vec3(0, 10, 0) - vpos;
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
        specular = pow(specAngle, 0.2);
    }
    vec3 colorLinear = vec3(1) +
    vcol * lambertian * vec3(1) * vec3(1) / distance +
    vcol * specular * vec3(1) * vec3(1) / distance;
    // apply gamma correction (assume ambientColor, diffuseColor and specColor
    // have been linearized, i.e. have no gamma correction in them)
    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / 2.2));
    // use the gamma corrected color in the fragment
    color = colorGammaCorrected;
}