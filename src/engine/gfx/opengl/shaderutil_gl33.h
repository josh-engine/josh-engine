//
// Created by Ember Lee on 3/9/24.
//

#ifndef JOSHENGINE_SHADERUTIL_GL33_H
#define JOSHENGINE_SHADERUTIL_GL33_H
#include <fstream>
#include <sstream>
#include <GLFW/glfw3.h>

GLuint loadShaderGL33(const std::string file_path, int target){
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
        std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(shaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    return shaderID;
}

GLuint createProgramGL33(GLuint VertexShaderID, GLuint FragmentShaderID){
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

#endif //JOSHENGINE_SHADERUTIL_GL33_H
