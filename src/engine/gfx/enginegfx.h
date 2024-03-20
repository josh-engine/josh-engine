//
// Created by Ethan Lee on 3/9/24.
//

#ifndef JOSHENGINE3_1_ENGINEGFX_H
#define JOSHENGINE3_1_ENGINEGFX_H
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <utility>
#include <vector>

class Renderable {
    public:
        bool enabled;
        bool is3d;
        std::vector<GLfloat> vertices;
        std::vector<GLfloat> colors;
        std::vector<GLfloat> uvs;
        std::vector<GLfloat> normals;
        std::vector<unsigned int> indices;
        GLuint shaderProgram;
        GLuint texture;
        //float alpha;
        glm::mat4 objectMatrix;
        Renderable(){
            enabled = false;
        }
        Renderable(bool _3d, std::vector<GLfloat> verts, std::vector<GLfloat> cols, std::vector<GLfloat> _uvs, std::vector<GLfloat> norms, std::vector<unsigned int> ind,/* float alpha, */GLuint shid, GLuint tex){
            enabled = true;
            is3d = _3d;
            vertices = std::move(verts);
            colors = std::move(cols);
            uvs = std::move(_uvs);
            normals = std::move(norms);
            indices = std::move(ind);
            shaderProgram = shid; // Once in a blue moon do you realize what you just named a variable... I'm keeping it.
            texture = tex;
            //this->alpha = alpha;
        }
        Renderable addMatrix(glm::mat4 matrix){
            Renderable r = Renderable(this->is3d, this->vertices, this->colors, this->uvs, this->normals, this->indices,/* this->alpha, */this->shaderProgram, this->texture);
            r.objectMatrix = matrix;
            return r;
        }
};

void initGFX(GLFWwindow** window);
void
renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, float fieldOfViewAngle, std::vector<Renderable> renderables,
            int i, int i1);
GLuint getProgram(std::string name);
void registerProgram(std::string name, std::string vertex, std::string fragment);
GLuint createTextureWithName(std::string name, std::string fileName);
GLuint createTexture(std::string folderPath, std::string fileName);
GLuint getTexture(std::string name);

#endif //JOSHENGINE3_1_ENGINEGFX_H
