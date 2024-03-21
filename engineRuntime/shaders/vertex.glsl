#version 330

// Per-vertex inputs
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec3 vertexNormal;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform mat4 translationMatrix;
uniform mat4 rotationMatrix;
uniform mat4 scaleMatrix;

// Send these off to frag shader
out vec3 vcol;
out vec2 UV;
out vec3 vnorm;
out vec3 vpos;

void main(){
    // Output position of the vertex, in clip space: MVP * position
    gl_Position =  MVP * vec4(vertexPosition_modelspace,1);
    vcol = vertexColor;
    vec4 pos = ((translationMatrix * rotationMatrix * scaleMatrix) * vec4(vertexPosition_modelspace,1));
    vpos = pos.xyz;
    UV = vertexUV;
    vec4 normalv4 = ((rotationMatrix) * vec4(vertexNormal,1));
    vnorm = normalv4.xyz;
}