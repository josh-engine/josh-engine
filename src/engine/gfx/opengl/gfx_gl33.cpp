#include "../../engineconfig.h"
#ifdef GFX_API_OPENGL33
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include "gfx_gl33.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../../../stb/stb_image.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

//
// Created by Ember Lee on 3/9/24.
//
GLuint vaoID;
GLuint vboID;
GLuint cboID;
GLuint tboID;
GLuint nboID;
GLuint iboID;
glm::vec3 ambient(glm::max(AMBIENT_RED - 0.5f, 0.1f), glm::max(AMBIENT_GREEN - 0.5f, 0.1f), glm::max(AMBIENT_BLUE - 0.5f, 0.1f));

GLuint loadCubemap(std::vector<std::string> faces) {
    stbi_set_flip_vertically_on_load(false);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cerr << "Failed to load cubemap texture " << faces[i] << "!" << std::endl;
            stbi_image_free(data);
            textureID = 0;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

GLuint loadTexture(std::string fileName){
    stbi_set_flip_vertically_on_load(true);
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Set up blending
    // UPDATE TRANSPARENCY IS EVIL AND FRAMERATE DROPS TO HELL
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef DO_SKYBOX
    glClearColor(AMBIENT_RED, AMBIENT_GREEN, AMBIENT_BLUE, CLEAR_ALPHA);
#endif
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

    // Set up ImGui
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

// Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(*window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
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

GLuint loadShader(const std::string file_path, int target){
    // Create the shader
    GLuint shaderID = glCreateShader(target);

    // Read the shader code
    std::string shaderCode;
    std::ifstream shaderCodeStream(file_path, std::ios::in);
    if(shaderCodeStream.is_open()){
        std::stringstream sstr;
        sstr << shaderCodeStream.rdbuf();
        shaderCode = sstr.str();
        shaderCodeStream.close();
    }else{
        std::cout << "Couldn't open \"" << file_path << "\", are you sure it exists?" << std::endl;
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Shader
    std::cout << "Compiling " << file_path << "..." << std::endl;
    char const * sourcePointer = shaderCode.c_str();
    glShaderSource(shaderID, 1, &sourcePointer, NULL);
    glCompileShader(shaderID);

    // Check Shader
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> shaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(shaderID, InfoLogLength, NULL, &shaderErrorMessage[0]);
        printf("\e[0;33m%s\e[0m\n", &shaderErrorMessage[0]);
    }

    return shaderID;
}

GLuint createProgram(GLuint VertexShaderID, GLuint FragmentShaderID){
    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Link the program
    std::cout << "Linking program..." << std::endl;
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    std::cout << "Testing program..." << std::endl;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        // \e characters are yellow escape and reset so warnings are different color
        printf("\e[0;33m%s\e[0m\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    std::cout << "Success!" << std::endl;

    return ProgramID;
}

void renderFrame(GLFWwindow **window, glm::mat4 cameraMatrix, glm::vec3 camerapos, float fieldOfViewAngle, std::vector<Renderable> renderables, int w, int h, std::vector<void (*)()> imGuiCalls) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    for (auto execute : imGuiCalls){
        execute();
    }

    glm::mat4 projectionMatrix;
    glm::mat4 mvp;
#ifndef DO_SKYBOX
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else //DO_SKYBOX
    glClear(GL_DEPTH_BUFFER_BIT);
#endif //DO_SKYBOX
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    unsigned int currentProgram = 0;

    for (auto renderable : renderables){
        if (renderable.enabled){
            if (renderable.testDepth){
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }

            if (renderable.is3d){
                projectionMatrix = glm::perspective(glm::radians(fieldOfViewAngle), (float) w / (float) h, 0.01f, 500.0f);
                mvp = projectionMatrix * cameraMatrix * renderable.objectMatrix();
            } else {
                float scaledHeight = h * (1.0f / w);
                float scaledWidth = 1.0f;
                projectionMatrix = glm::ortho(-scaledWidth,scaledWidth,-scaledHeight,scaledHeight,0.0f,100.0f); // In world coordinates
                #ifdef CAMERA_AFFECTS_2D
                mvp = projectionMatrix * cameraMatrix * renderable.objectMatrix();
                #else
                mvp = projectionMatrix * renderable.objectMatrix();
                #endif
            }

            if (renderable.shaderProgram != currentProgram){
                glUseProgram(renderable.shaderProgram);
                currentProgram = renderable.shaderProgram;
            }

            GLint MatrixID = glGetUniformLocation(renderable.shaderProgram, "MVP");
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

            GLint translate = glGetUniformLocation(renderable.shaderProgram, "translationMatrix");
            glUniformMatrix4fv(translate, 1, GL_FALSE, &renderable.transform[0][0]);

            GLint rotate = glGetUniformLocation(renderable.shaderProgram, "rotationMatrix");
            glUniformMatrix4fv(rotate, 1, GL_FALSE, &renderable.rotate[0][0]);

            GLint scale = glGetUniformLocation(renderable.shaderProgram, "scaleMatrix");
            glUniformMatrix4fv(scale, 1, GL_FALSE, &renderable.scale[0][0]);

            GLint camera = glGetUniformLocation(renderable.shaderProgram, "cameraPosition");
            glUniform3fv(camera, 1, &camerapos[0]);

            GLint amb = glGetUniformLocation(renderable.shaderProgram, "ambience");
            glUniform3fv(amb, 1, &ambient[0]);

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
                    0,                  // attribute
                    3,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
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
                    1,                                // attribute
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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(*window);
}

void deinitGFX(GLFWwindow** window){
    glfwDestroyWindow(*window);
    glfwTerminate();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif //GFX_API_OPENGL33