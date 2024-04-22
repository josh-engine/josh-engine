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

#ifdef GFX_API_VK
#include "vk/gfx_vk.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO((Renderable*)r);
}

#endif

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