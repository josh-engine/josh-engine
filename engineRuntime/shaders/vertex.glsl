#version 330

// Per-vertex inputs
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec3 vertexNormal;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

// Send these off to frag shader
out vec3 vcol;
out vec2 UV;
out vec3 vnorm;
out vec3 vpos;

void main(){
    // Output position of the vertex, in clip space: MVP * position
    gl_Position =  MVP * vec4(vertexPosition_modelspace,1);
    vcol = vertexColor;
    vpos = vec3(gl_Position.x, gl_Position.y, gl_Position.z);
    UV = vertexUV;
    vnorm = vertexNormal;
}