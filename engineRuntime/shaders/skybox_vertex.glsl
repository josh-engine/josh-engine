// JE_TRANSLATE
#version 420

layout (location = 0) in vec3 vpos;

layout (location = 0) out vec3 coords;

layout(binding = 0) uniform UBO { // JE_TRANSLATE
                                  mat4 viewMatrix;
                                  mat4 _2dProj;
                                  mat4 _3dProj;
                                  vec3 cameraPos;
                                  vec3 cameraDir;
                                  vec3 ambience;
};

void main()
{
    coords = vpos;
    gl_Position = _3dProj * vec4(vpos, 1.0);
}