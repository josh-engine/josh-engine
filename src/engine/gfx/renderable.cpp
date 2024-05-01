//
// Created by Ember Lee on 3/27/24.
//
#include <fstream>
#include <istream>
#include <filesystem>
#include <iostream>
#include <iterator>
#include "renderable.h"
#include "../engine.h"
#include <bit>

#ifdef GFX_API_OPENGL41
#ifdef PLATFORM_MAC
#include <OpenGL/gl3.h>
#elif defined(PLATFORM_UNIX)
#include <GL/gl.h>
#endif
#endif

#ifdef GFX_API_VK
#include "vk/gfx_vk.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO(reinterpret_cast<Renderable*>(r));
}
#endif

#ifdef GFX_API_MTL
#include "mtl/gfx_mtl.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO(reinterpret_cast<Renderable*>(r));
}
#endif

Renderable::Renderable(std::vector<float> verts, std::vector<float> _uvs, std::vector<float> norms,
                       std::vector<unsigned int> ind, unsigned int shid, unsigned int tex, bool manualDepthSort) {
    enabled = true;
    vertices = std::move(verts);
    uvs = std::move(_uvs);
    normals = std::move(norms);
    indices = std::move(ind);
    shaderProgram = shid; // Once in a blue moon do you realize what you just named a variable... I'm keeping it.
    texture = tex;
    this->manualDepthSort = manualDepthSort;

#ifdef GFX_API_OPENGL41
    // Vertex Buffer
    glGenBuffers(1, &vboID); // reserve an ID for our VBO
    glBindBuffer(GL_ARRAY_BUFFER, vboID); // bind VBO
    glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(float),
            &vertices[0],
            GL_STATIC_DRAW
    );


    // Texture Coordinate Buffer
    glGenBuffers(1, &tboID);
    glBindBuffer(GL_ARRAY_BUFFER, tboID);
    glBufferData(
            GL_ARRAY_BUFFER,
            uvs.size() * sizeof(float),
            &uvs[0],
            GL_STATIC_DRAW
    );

    // Normals Buffer
    glGenBuffers(1, &nboID);
    glBindBuffer(GL_ARRAY_BUFFER, nboID);
    glBufferData(
            GL_ARRAY_BUFFER,
            normals.size() * sizeof(float),
            &normals[0],
            GL_STATIC_DRAW
    );

    // Indices Buffer
    glGenBuffers(1, &iboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            &indices[0],
            GL_STATIC_DRAW
    );
#endif //GFX_API_OPENGL41
#if defined(GFX_API_VK) | defined(GFX_API_MTL)
    for (int i = 0; i < vertices.size()/3; i++) {
            interleavedVertices.push_back({
                                                  {vertices[3*i],  vertices[(3*i)+1], vertices[(3*i)+2]},
                                                  {uvs[(2*i)],     uvs[(2*i)+1]},
                                                  {normals[(3*i)], normals[(3*i)+1],  normals[(3*i)+2]}
            });
        }

        vboID = createVBOFunctionMirror(this);
#endif
    indicesSize = indices.size();
}

void writeDWordLE(glm::uint64 DWord, std::fstream& file) {
    file.put((char)(DWord));
    file.put((char)(DWord >> 8));
    file.put((char)(DWord >> 16));
    file.put((char)(DWord >> 24));
}

void writeDWordBE(glm::uint64 DWord, std::fstream& file) {
    file.put((char)(DWord >> 24));
    file.put((char)(DWord >> 16));
    file.put((char)(DWord >> 8));
    file.put((char)(DWord));
}

void writeFloatVec(const std::vector<float>& vec, std::fstream& file) {
    writeDWordLE(static_cast<unsigned int>(vec.size()), file);
    for (float const& i : vec){
        // evil bit cast a la fast inverse square root
        writeDWordLE(std::bit_cast<unsigned>(i), file);
    }
}

void Renderable::saveToFile(const std::string& fileName) {
    std::fstream file;
    std::filesystem::path p = fileName;
    resize_file(p, 0);

    file.open(fileName, std::fstream::in | std::fstream::out | std::fstream::app);
    file.seekg(0, std::ios::beg);

    // Bad File (idk couldn't come up with another magic number)
    writeDWordBE(0x0BADF115, file); // Magic number looks better in big endian in a hex editor

    std::string txname = textureReverseLookup(this->texture);
    writeDWordLE(txname.size(), file);
    for (char c : txname) {
        file.put(c);
    }
    std::string prgname = programReverseLookup(this->shaderProgram);
    writeDWordLE(prgname.size(), file);
    for (char c : prgname) {
        file.put(c);
    }
    writeFloatVec(this->vertices, file);
    writeFloatVec(this->uvs, file);
    writeFloatVec(this->normals, file);

    writeDWordLE(indicesSize, file);
    for (unsigned int & i : indices){
        writeDWordLE(i, file);
    }

    file.put(static_cast<char>(this->manualDepthSort));

    file.close();
}

glm::uint32 readDWordBE(std::vector<unsigned char>& file, unsigned int* offset){
    glm::uint32 staging = 0;
    staging |= file[*offset  ] << 24;
    staging |= file[*offset+1] << 16;
    staging |= file[*offset+2] << 8;
    staging |= file[*offset+3];
    *offset += 4;
    return staging;
}

glm::uint32 readDWordLE(std::vector<unsigned char>& file, unsigned int* offset){
    glm::uint32 staging = 0;
    staging |= file[*offset  ];
    staging |= file[*offset+1] << 8;
    staging |= file[*offset+2] << 16;
    staging |= file[*offset+3] << 24;
    *offset += 4;
    return staging;
}

std::unordered_map<std::string, Renderable> onlyLoadOnce;

Renderable renderableFromFile(const std::string& fileName) {
    if (onlyLoadOnce.contains(fileName)) {
        return onlyLoadOnce.at(fileName);
    }
    std::ifstream file(fileName, std::ios::binary);
    std::vector<unsigned char> fileBytes( // StackOverflow yoink (https://stackoverflow.com/questions/50315742/how-do-i-read-bytes-from-file-c)
            (std::istreambuf_iterator<char>(file)),
            (std::istreambuf_iterator<char>()));
    file.close();

    if (fileBytes.empty()) {
        std::cerr << "Could not load renderable file \"" << fileName << "\" (no data)" << std::endl;
        return {};
    }

    unsigned int offset = 0;

    if (readDWordBE(fileBytes, &offset) != 0x0BADF115) {
        std::cerr << "Could not load renderable file \"" << fileName << "\" (invalid magic number)" << std::endl;
        return {};
    }

    unsigned int textureNameLength = readDWordLE(fileBytes, &offset);
    std::string txname;
    for (int i = 0; i < textureNameLength; i++){
        txname.push_back(std::bit_cast<char>(fileBytes[offset]));
        offset++;
    }
    unsigned int textureID = getTexture(txname);

    unsigned int shaderProgramNameLength = readDWordLE(fileBytes, &offset);
    std::string pgname;
    for (int i = 0; i < shaderProgramNameLength; i++){
        pgname.push_back(std::bit_cast<char>(fileBytes[offset]));
        offset++;
    }
    unsigned int programID = getProgram(pgname);

    std::vector<float> vertices = {};
    unsigned int vertexLength = readDWordLE(fileBytes, &offset);
    vertices.reserve(vertexLength);
    for (int i = 0; i < vertexLength; i++){
        vertices.push_back(std::bit_cast<float>(readDWordLE(fileBytes, &offset)));
    }

    std::vector<float> uv = {};
    unsigned int uvLength = readDWordLE(fileBytes, &offset);
    uv.reserve(uvLength);
    for (int i = 0; i < uvLength; i++){
        uv.push_back(std::bit_cast<float>(readDWordLE(fileBytes, &offset)));
    }

    std::vector<float> normals = {};
    unsigned int normalLength = readDWordLE(fileBytes, &offset);
    normals.reserve(normalLength);
    for (int i = 0; i < normalLength; i++){
        normals.push_back(std::bit_cast<float>(readDWordLE(fileBytes, &offset)));
    }

    std::vector<unsigned int> indicies = {};
    unsigned int indiciesLength = readDWordLE(fileBytes, &offset);
    indicies.reserve(indiciesLength);
    for (int i = 0; i < indiciesLength; i++){
        indicies.push_back(readDWordLE(fileBytes, &offset));
    }

    bool manualDepthSort = fileBytes[offset];

    Renderable r = {vertices, uv, normals, indicies, programID, textureID, manualDepthSort};
    onlyLoadOnce.insert({fileName, r});
    return r;
}