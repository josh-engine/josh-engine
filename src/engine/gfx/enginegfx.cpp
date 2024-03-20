#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include "../engineconfig.h"
#include "shaderutil.h"
#include "enginegfx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../stb/stb_image.h"
using namespace glm;

//
// Created by Ethan Lee on 3/9/24.
//
GLuint vaoID;
GLuint vboID;
GLuint cboID;
GLuint tboID;
GLuint nboID;
GLuint iboID;
GLuint vertID;
GLuint fragID;
std::unordered_map<std::string, GLuint> programs;
std::unordered_map<std::string, GLuint> textures;

void registerProgram(std::string name, std::string vertex, std::string fragment) {
    vertID = loadShader(std::move(vertex), GL_VERTEX_SHADER);
    fragID = loadShader(std::move(fragment), GL_FRAGMENT_SHADER);
    programs.insert({name, createProgram(vertID, fragID)});
}

GLuint getProgram(std::string name){
    return programs.at(name);
}

GLuint loadTexture(std::string fileName){
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA-4+nrChannels, width, height, 0, GL_RGBA-4+nrChannels, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture " << fileName << "!" << std::endl;
        texture = 0;
    }
    stbi_image_free(data);
    return texture;
}

GLuint createTextureWithName(std::string name, std::string filePath){
    GLuint id = loadTexture(std::move(filePath));
    if (id != 0){
        textures.insert({name, id});
    }
    return id;
}

GLuint createTexture(std::string folderPath, std::string fileName){
    GLuint id = loadTexture(folderPath + fileName);
    if (id != 0){
        textures.insert({fileName, id});
    }
    return id;
}

GLuint getTexture(std::string name){
    if (!textures.count(name)) {
        std::cerr << "Texture \"" + name + "\" not found, defaulting to missing_tex.png" << std::endl;
        return textures.at("missing");
    }
    return textures.at(name);
}

void initGFX(GLFWwindow** window){
    stbi_set_flip_vertically_on_load(true);

    if(!glfwInit())
    {
        std::cout << "Failed to init GLFW!" << std::endl;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // will use 4.0+ eventually :tm:
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fucking macOS

    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, nullptr, nullptr);
    glfwSetCursorPos(*window, WINDOW_WIDTH/2, WINDOW_HEIGHT/2);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    if(*window == nullptr){
        std::cout << "Failed to open window!" << std::endl;
        glfwTerminate();
    }

    glfwMakeContextCurrent(*window);
    glewExperimental = true; // glew is on some crack or something, needed this to compile

    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to init GLEW!" << std::endl;
    }

    // Set up depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Backface culling
#ifdef BACKFACE_CULL
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
#endif

    // Set up blending
    // UPDATE TRANSPARENCY IS EVIL AND FRAMERATE DROPS TO HELL
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(CLEAR_RED, CLEAR_BLUE, CLEAR_GREEN, CLEAR_ALPHA);

    // Vertex Array
    glGenVertexArrays(1, &vaoID); //reserve an ID for our VAO
    glBindVertexArray(vaoID); // bind VAO

    // Vertex Buffer
    glGenBuffers(1, &vboID); // reserve an ID for our VBO
    glBindBuffer(GL_ARRAY_BUFFER, vboID); // bind VBO

    // Color Buffer
    glGenBuffers(1, &cboID);
    glBindBuffer(GL_ARRAY_BUFFER, cboID);

    // Texture Coordinate Buffer
    glGenBuffers(1, &tboID);
    glBindBuffer(GL_ARRAY_BUFFER, tboID);

    // Normals Buffer
    glGenBuffers(1, &nboID);
    glBindBuffer(GL_ARRAY_BUFFER, nboID);

    // Indices Buffer
    glGenBuffers(1, &iboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);

    createTextureWithName("missing", "./textures/missing_tex.png");
    if (!textures.count("missing")){
        std::cerr << "Essential engine file missing." << std::endl;
        exit(1);
    }
}

/*
static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f, 1.0f,
         1.0f, -1.0f, 1.0f,
        -1.0f,  1.0f, 1.0f
};

static const GLfloat g_color_buffer_data[] = {
        1.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,
        0.0f,  0.0f,  1.0f
};

static const GLfloat g_uv_buffer_data[] = {
        0.0f,  0.0f,
        1.0f,  0.0f,
        0.0f,  1.0f
};
 */

void renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, float fieldOfViewAngle, std::vector<Renderable> renderables, int w, int h) {
    glm::mat4 projectionMatrix;
    glm::mat4 mvp;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    for (auto renderable : renderables){
        if (renderable.enabled){
            if (renderable.is3d){
                projectionMatrix = glm::perspective(glm::radians(fieldOfViewAngle), (float) w / (float)h, 0.01f, 500.0f);
                mvp = projectionMatrix * cameraMatrix * renderable.objectMatrix;
            } else {
                float scaledHeight = h * (1.0f/w);
                float scaledWidth = 1.0;
                projectionMatrix = glm::ortho(-scaledWidth,scaledWidth,-scaledHeight,scaledHeight,0.0f,100.0f); // In world coordinates
                #ifdef CAMERA_AFFECTS_2D
                mvp = projectionMatrix * cameraMatrix * renderable.objectMatrix;
                #else
                mvp = projectionMatrix * renderable.objectMatrix;
                #endif
            }

            glUseProgram(renderable.shaderProgram);

            GLint MatrixID = glGetUniformLocation(renderable.shaderProgram, "MVP");
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
            //GLint alphaID = glGetUniformLocation(renderable.shaderProgram, "alpha");
            //glUniform1f(alphaID, renderable.alpha);

            glBindTexture(GL_TEXTURE_2D, renderable.texture);

            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            glBufferData(
                    GL_ARRAY_BUFFER,
                    renderable.vertices.size() * sizeof(GLfloat),
                    &renderable.vertices[0],
                    GL_STATIC_DRAW
            );
            glVertexAttribPointer(
                    0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                    3,                  // each attribute is 3 wide
                    GL_FLOAT,           // these are floats
                    GL_FALSE,           // not normalized
                    0,                  // stride
                    (void*)0            // array buffer offset
            );


            glBindBuffer(GL_ARRAY_BUFFER, cboID);
            glBufferData(
                    GL_ARRAY_BUFFER,
                    renderable.colors.size() * sizeof(GLfloat),
                    &renderable.colors[0],
                    GL_STATIC_DRAW
            );
            glVertexAttribPointer(
                    1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
            );

            glBindBuffer(GL_ARRAY_BUFFER, tboID);
            glBufferData(
                    GL_ARRAY_BUFFER,
                    renderable.uvs.size() * sizeof(GLfloat),
                    &renderable.uvs[0],
                    GL_STATIC_DRAW
            );
            glVertexAttribPointer(
                    2,                                // attribute
                    2,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
            );

            glBindBuffer(GL_ARRAY_BUFFER, nboID);
            glBufferData(
                    GL_ARRAY_BUFFER,
                    renderable.normals.size() * sizeof(GLfloat),
                    &renderable.normals[0],
                    GL_STATIC_DRAW
            );
            glVertexAttribPointer(
                    3,                                // attribute
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    (void*)0                          // array buffer offset
            );

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
            glBufferData(
                    GL_ELEMENT_ARRAY_BUFFER,
                    renderable.indices.size() * sizeof(unsigned int),
                    &renderable.indices[0],
                    GL_STATIC_DRAW
                    );

            // Draw the cube_face !
            glDrawElements(
                    GL_TRIANGLES,      // mode
                    renderable.indices.size(),    // count
                    GL_UNSIGNED_INT,   // type
                    (void*)0           // element array buffer offset
            );
        }
    }

    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    // Projection matrix: 78° Field of View, WW:WH ratio, display range: 0.1 unit <-> 100 units

    // 2D (ortho matrix)

    // Model matrix: an identity matrix (model will be at the origin)

    // Our ModelViewProjection: multiplication of our 3 matrices

    // Get a handle for our "MVP" uniform
    // Only during the initialisation

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // This is done in the main loop since each model will have a different MVP matrix (At least for the M part)

    glfwSwapBuffers(*window);
}