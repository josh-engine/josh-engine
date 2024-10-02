// JE_TRANSLATE
#version 420

// Interpolated values from the vertex shaders
layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 uv;
layout(location = 2) flat in uint character;
layout(location = 3) in vec3 fontcolor;

// Ouput data
layout(location = 0) out vec4 color;

// Values that stay constant for the whole mesh.
layout(set = 1, binding = 0) uniform sampler2D textureSampler;

void main() {
    vec2 charUV = vec2((float(character % 16) / 16), ((floor((character) / 16)) / 16));
    vec2 uv2 = vec2(uv.x/16 + charUV.x, (1 - (uv.y/16) - charUV.y));
    if (
    texture(textureSampler, uv2).r == 1.0f &&
    uv.y > 0 &&
    uv.y < 1 &&
    character != 32
    )
    {
        color = vec4(fontcolor, 1);
    }
    else discard;
}