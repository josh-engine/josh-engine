#ifdef GFX_API_OPENGL41
#include "gfx_gl41.h"
#include <iostream>
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

GLFWwindow** windowPtr;

// Same ID system as used in Vulkan implementation, but used atop OpenGL now. The irony is wild.
std::vector<JEShaderProgram_GL41> shaderProgramVector;

void resizeViewport() {
    int width, height;
    glfwGetFramebufferSize(*windowPtr, &width, &height);
    glViewport(0, 0, width, height);
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    stbi_set_flip_vertically_on_load(false);
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
#ifdef GL41_SRGB
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
#else
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
#endif
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

unsigned int loadTexture(const std::string& fileName) {
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
#ifdef GL41_SRGB
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8-4+nrChannels, width, height, 0, GL_RGBA-4+nrChannels, GL_UNSIGNED_BYTE, data);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA-4+nrChannels, width, height, 0, GL_RGBA-4+nrChannels, GL_UNSIGNED_BYTE, data);
#endif
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

void initGFX(GLFWwindow **window, const char* windowName, int width, int height, JEGraphicsSettings graphicsSettings) {
    stbi_set_flip_vertically_on_load(true);
    windowPtr = window;

    if(!glfwInit())
    {
        throw std::runtime_error("OpenGL 4.1: Could not initialize GLFW!");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fucking macOS
    if (graphicsSettings.msaaSamples > 1) glfwWindowHint(GLFW_SAMPLES, graphicsSettings.msaaSamples);

    *windowPtr = glfwCreateWindow(width, height, windowName, nullptr, nullptr);
    glViewport(0, 0, width, height);

    if(*windowPtr == nullptr) {
        throw std::runtime_error("OpenGL 4.1: Could not open window!");
    }

    glfwMakeContextCurrent(*windowPtr);

    glfwSwapInterval(graphicsSettings.vsyncEnabled);

    // Set up depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Set up blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef GL41_SRGB
    // Enable SRGB framebuffer to attempt Vulkan parity
    glEnable(GL_FRAMEBUFFER_SRGB);
#endif

    if (graphicsSettings.msaaSamples > 1) glEnable(GL_MULTISAMPLE);

    glClearColor(graphicsSettings.clearColor[0], graphicsSettings.clearColor[1], graphicsSettings.clearColor[2], 1.0f);
    // Vertex Array
    // local function member since this thing is pretty much just fire and forget type beat in our case
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID); //reserve an ID for our VAO
    glBindVertexArray(vaoID); // bind VAO

    // set up VAO attributes
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Set up ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(*windowPtr, true);
    ImGui_ImplOpenGL3_Init();
}

unsigned int loadShader(const std::string& file_path, int target) {
    // Create the shader
    unsigned int shaderID = glCreateShader(target);

    // Read the shader code
    std::string shaderCode;
    std::ifstream shaderCodeStream(file_path, std::ios::in);
    if (shaderCodeStream.is_open()) {
        std::stringstream sstr;
        sstr << shaderCodeStream.rdbuf();
        shaderCode = sstr.str();
        shaderCodeStream.close();
    } else {
        std::cerr << "Couldn't open \"" << file_path << "\", are you sure it exists?" << std::endl;
        return 0;
    }

    if (shaderCode.starts_with("// JE_TRANSLATE\n#version 420")) {
        std::cout << "Translating " << file_path << "... (JE_TRANSLATE, Vulkan GLSL 420 -> OpenGL GLSL 410)" << std::endl;
        std::string tempShaderCode = shaderCode;
        tempShaderCode.replace(0, 28, "#version 410");
        std::vector<std::string> lines;
        std::string line;
        for (char character : tempShaderCode) {
            if (character != '\n')
                line += character;
            else {
                lines.push_back(line);
                line = "";
            }
        }
        lines.push_back(line);
        shaderCode = "";
        for (int i = 0; i < lines.size(); i++) {
            line = lines[i];
            if (line.starts_with("layout") && (line.find("uniform") != std::string::npos)) {
                if ((line.find("JE_TRANSLATE") != std::string::npos)) {
                    line = lines[++i];
                    do {
                        line.replace(0, 0, "uniform");
                        shaderCode.append(line + "\n");
                        line = lines[++i];
                    } while (!line.starts_with('}'));
                    continue;
                } else {
                    unsigned long layoutEnd = line.find("uniform");
                    line.replace(0, layoutEnd, "");
                }
            }
            shaderCode.append(line + "\n");
        }
    }

    int InfoLogLength;

    // Compile Shader
    std::cout << "Compiling " << file_path << "..." << std::endl;
    char const * sourcePointer = shaderCode.c_str();
    glShaderSource(shaderID, 1, &sourcePointer, nullptr);
    glCompileShader(shaderID);

    // Check Shader
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        std::vector<char> shaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(shaderID, InfoLogLength, nullptr, &shaderErrorMessage[0]);
        printf("\e[0;33m%s\e[0m\n", &shaderErrorMessage[0]);
    }

    return shaderID;
}

unsigned int createProgram(unsigned int VertexShaderID, unsigned int FragmentShaderID, bool testDepth, bool transparencySupported, bool doubleSided) {
    int InfoLogLength;

    // Link the program
    std::cout << "Linking program..." << std::endl;
    unsigned int ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    std::cout << "Testing program..." << std::endl;
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
        // \e characters are yellow escape and reset so warnings are different color
        printf("\e[0;33m%s\e[0m\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    std::cout << "Success!" << std::endl;

    unsigned int id = shaderProgramVector.size();
    shaderProgramVector.emplace_back(ProgramID, testDepth, transparencySupported, doubleSided);

    return id;
}

// Slight performance boost, OpenGL's state machine persists between renderFrame calls.
int currentTexture = -1;
bool currentDepth = false, transparency = false, backfaceCull = true;

void renderFrame(glm::vec3 camerapos, glm::vec3 cameradir, glm::vec3 sundir, glm::vec3 suncol, glm::vec3 ambient, glm::mat4 cameraMatrix,  glm::mat4 _2dProj, glm::mat4 _3dProj, const std::vector<Renderable>& renderables, const std::vector<void (*)()>& imGuiCalls) {
    // Disable transparency so we don't fail clearing the depth buffer
    if (transparency){
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        transparency = false;
    }

    // Can't persist program because uniforms change each frame.
    int currentProgram = -1;

    // According to Khronos.org's wiki page, clearing both buffers
    // will always be faster even while drawing skybox.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    for (auto execute : imGuiCalls) {
        execute();
    }

    ImGui::Render();

    for (auto r : renderables) {
        if (r.enabled) {
            if (shaderProgramVector[r.shaderProgram].testDepth ^ currentDepth) {
                if (shaderProgramVector[r.shaderProgram].testDepth) {
                    glEnable(GL_DEPTH_TEST);
                } else {
                    glDisable(GL_DEPTH_TEST);
                }
                currentDepth = shaderProgramVector[r.shaderProgram].testDepth;
            }

            if (shaderProgramVector[r.shaderProgram].doubleSided == backfaceCull){
                if (shaderProgramVector[r.shaderProgram].doubleSided) {
                    glDisable(GL_CULL_FACE);
                } else {
                    glEnable(GL_CULL_FACE);
                }
                backfaceCull = !shaderProgramVector[r.shaderProgram].doubleSided;
            }

            if (shaderProgramVector[r.shaderProgram].transparencySupported ^ transparency){
                if (shaderProgramVector[r.shaderProgram].transparencySupported){
                    glEnable(GL_BLEND);
                    glDepthMask(GL_FALSE);
                } else {
                    glDisable(GL_BLEND);
                    glDepthMask(GL_TRUE);
                }
                transparency = shaderProgramVector[r.shaderProgram].transparencySupported;
            }

            if (shaderProgramVector[r.shaderProgram].glShaderProgramID != currentProgram) {
                glUseProgram(shaderProgramVector[r.shaderProgram].glShaderProgramID);

                // We only need to bind per-frame uniforms once per shader
                glUniformMatrix4fv(shaderProgramVector[r.shaderProgram].location_view,      1, GL_FALSE, &cameraMatrix[0][0]);
                glUniformMatrix4fv(shaderProgramVector[r.shaderProgram].location_2dProj,    1, GL_FALSE, &_2dProj[0][0]     );
                glUniformMatrix4fv(shaderProgramVector[r.shaderProgram].location_3dProj,    1, GL_FALSE, &_3dProj[0][0]     );
                glUniform3fv      (shaderProgramVector[r.shaderProgram].location_cameraPos, 1,           &camerapos[0]      );
                glUniform3fv      (shaderProgramVector[r.shaderProgram].location_cameraDir, 1,           &cameradir[0]      );
                glUniform3fv      (shaderProgramVector[r.shaderProgram].location_sunDir,    1,           &sundir[0]         );
                glUniform3fv      (shaderProgramVector[r.shaderProgram].location_sunColor,  1,           &suncol[0]         );
                glUniform3fv      (shaderProgramVector[r.shaderProgram].location_ambience,  1,           &ambient[0]        );

                currentProgram = static_cast<int>(shaderProgramVector[r.shaderProgram].glShaderProgramID);
            }

            if (r.texture != currentTexture) {
                glBindTexture(GL_TEXTURE_2D, r.texture);
                currentTexture = static_cast<int>(r.texture);
            }

            glUniformMatrix4fv(shaderProgramVector[r.shaderProgram].location_model,  1, GL_FALSE, &r.objectMatrix[0][0]);
            glUniformMatrix4fv(shaderProgramVector[r.shaderProgram].location_normal, 1, GL_FALSE, &r.rotate[0][0]      );

            glBindBuffer(GL_ARRAY_BUFFER, r.vboID);
            glVertexAttribPointer(
                    0,                  // attribute
                    3,                  // size
                    GL_FLOAT,           // type
                    GL_FALSE,           // normalized?
                    0,                  // stride
                    nullptr
            );

            glBindBuffer(GL_ARRAY_BUFFER, r.tboID);
            glVertexAttribPointer(
                    1,                                // attribute
                    2,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    0,                                // stride
                    nullptr
            );

            glBindBuffer(GL_ARRAY_BUFFER, r.nboID);
            glVertexAttribPointer(
                    2,                                // attribute
                    3,                                // size
                    GL_FLOAT,                         // type
                    GL_TRUE,                          // normalized?
                    0,                                // stride
                    nullptr
            );

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.iboID);

            glDrawElements(
                    GL_TRIANGLES,      // mode
                    static_cast<int>(r.indicesSize),    // count
                    GL_UNSIGNED_INT,   // type
                    nullptr           // element array buffer offset
            );
        }
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(*windowPtr);
}

void deinitGFX() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(*windowPtr);
    glfwTerminate();
}
#endif //GFX_API_OPENGL41