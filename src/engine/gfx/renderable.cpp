//
// Created by Ember Lee on 3/27/24.
//
#include <fstream>
#include <istream>
#include <filesystem>
#include <iostream>
#include <iterator>

#ifdef GFX_API_VK
#include "vk/gfx_vk.h"

unsigned int createVBOFunctionMirror(void* r) {
    return createVBO((Renderable*)r);
}

#endif

void writeDWordLE(uint64 DWord, std::fstream& file) {
    file.put((char)(DWord));
    file.put((char)(DWord >> 8));
    file.put((char)(DWord >> 16));
    file.put((char)(DWord >> 24));
}

void writeDWordBE(uint64 DWord, std::fstream& file) {
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
    writeDWordLE(this->texture, file);
    writeDWordLE(this->shaderProgram, file);

    writeFloatVec(this->vertices, file);
    writeFloatVec(this->colors, file);
    writeFloatVec(this->uvs, file);
    writeFloatVec(this->normals, file);

    writeDWordLE(indicesSize, file);
    for (unsigned int & i : indices){
        writeDWordLE(i, file);
    }

    file.put(static_cast<char>(this->manualDepthSort));

    file.close();
}

uint32 readDWordBE(std::vector<unsigned char>& file, unsigned int offset){
    uint32 staging = 0;
    staging |= file[offset  ] << 24;
    staging |= file[offset+1] << 16;
    staging |= file[offset+2] << 8;
    staging |= file[offset+3];
    return staging;
}

uint32 readDWordLE(std::vector<unsigned char>& file, unsigned int offset){
    uint32 staging = 0;
    staging |= file[offset  ];
    staging |= file[offset+1] << 8;
    staging |= file[offset+2] << 16;
    staging |= file[offset+3] << 24;
    return staging;
}

Renderable renderableFromFile(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary);
    std::vector<unsigned char> fileBytes( // StackOverflow yoink (https://stackoverflow.com/questions/50315742/how-do-i-read-bytes-from-file-c)
            (std::istreambuf_iterator<char>(file)),
            (std::istreambuf_iterator<char>()));
    file.close();

    if (fileBytes.empty()) {
        std::cerr << "Could not load renderable file \"" << fileName << "\" (no data)" << std::endl;
        return {};
    }

    if (readDWordBE(fileBytes, 0) != 0x0BADF115) {
        std::cerr << "Could not load renderable file \"" << fileName << "\" (invalid magic number)" << std::endl;
        return {};
    }

    unsigned int texture = readDWordLE(fileBytes, 4);
    unsigned int shaderProgram = readDWordLE(fileBytes, 8);

    std::vector<float> vertices = {};
    unsigned int vertexLength = readDWordLE(fileBytes, 12);
    vertices.reserve(vertexLength);
    for (int i = 0; i < vertexLength; i++){
        vertices.push_back(std::bit_cast<float>(readDWordLE(fileBytes, (i+4)*4)));
    }

    std::vector<float> colors = {};
    unsigned int colorLength = readDWordLE(fileBytes, (4+vertexLength)*4);
    colors.reserve(colorLength);
    for (int i = 0; i < colorLength; i++){
        colors.push_back(std::bit_cast<float>(readDWordLE(fileBytes, (i+5+vertexLength)*4)));
    }

    std::vector<float> uv = {};
    unsigned int uvLength = readDWordLE(fileBytes, (5+vertexLength+colorLength)*4);
    uv.reserve(uvLength);
    for (int i = 0; i < uvLength; i++){
        uv.push_back(std::bit_cast<float>(readDWordLE(fileBytes, (i+6+vertexLength+colorLength)*4)));
    }

    std::vector<float> normals = {};
    unsigned int normalLength = readDWordLE(fileBytes, (6+vertexLength+colorLength+uvLength)*4);
    normals.reserve(normalLength);
    for (int i = 0; i < normalLength; i++){
        normals.push_back(std::bit_cast<float>(readDWordLE(fileBytes, (i+7+vertexLength+colorLength+uvLength)*4)));
    }

    std::vector<unsigned int> indicies = {};
    unsigned int indiciesLength = readDWordLE(fileBytes, (7+vertexLength+colorLength+uvLength+normalLength)*4);
    indicies.reserve(indiciesLength);
    for (int i = 0; i < indiciesLength; i++){
        indicies.push_back(readDWordLE(fileBytes, (i+8+vertexLength+colorLength+uvLength+normalLength)*4));
    }

    bool manualDepthSort = fileBytes[fileBytes.size()-1];

    return {vertices, colors, uv, normals, indicies, shaderProgram, texture, manualDepthSort};
}