//
// Created by Ember Lee on 3/23/24.
//
#include <fstream>
#include <sstream>
#include <iostream>
#include "renderable.h"
#include "../engine.h"

Renderable createQuad(bool is3d, unsigned int shader, unsigned int texture){
    return Renderable(
            is3d, //3d?
            {-1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f},    //verts
            { 1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f},   //colors
            { 0.0f,  0.0f,          1.0f,  0.0f,           0.0f,  1.0f       ,  1.0f,  1.0f},  //UVs
            { 0.0f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,    0.0f,  0.0f, -1.0f,  0.0f,  0.0f, -1.0f}, //normals
            { 0, 1, 2,     1, 3, 2 }, //indices
            shader,    //shader program
            texture,    //texture
            true
    );
}

class repackVertexObject {
public:
    glm::vec3 pos;
    glm::vec3 col;
    glm::vec2 uvs;
    glm::vec3 nml;

    repackVertexObject(glm::vec3 position, glm::vec3 colors, glm::vec2 texcoords, glm::vec3 normals){
        pos = position;
        col = colors;
        uvs = texcoords;
        nml = normals;
    }

    bool equals(repackVertexObject r){
        return r.pos == pos && r.col == col && r.uvs == uvs && r.nml == nml;
    }
};

Renderable repackRenderable(Renderable r){
    std::vector<repackVertexObject> repackList = {};
    std::vector<unsigned int> indices = {};

    // Generate unique lists
    for (int i = 0; i < r.vertices.size()/3; i++){
        // new vertex object with current properties
        repackVertexObject currentVertexObject(
                {r.vertices[i*3], r.vertices[(i*3)+1], r.vertices[(i*3)+2]},
                {r.colors[i*3], r.colors[(i*3)+1], r.colors[(i*3)+2]},
                {r.uvs[i*2], r.uvs[(i*2)+1]},
                {r.normals[i*3], r.normals[(i*3)+1], r.normals[(i*3)+2]});

        // assume not duplicate
        bool add = true;
        for (int j = 0; j < repackList.size(); j++){
            if (currentVertexObject.equals(repackList[j++])){
                // if this is a duplicate, don't add it to the list and push the current index in repack
                add = false;
                // There's probably a reason j-1 works.
                // I don't know what that reason is in any way shape or form.
                // I typed it in to see what would happen. It fixed it.
                // It might have to do with OBJ's indices starting at 1.
                // Nobody but God himself knows.
                // Don't touch this line. Please.
                indices.push_back(j-1);
                break;
            }
        }
        if (add) {
            indices.push_back(repackList.size());
            repackList.push_back(currentVertexObject);
        }
    }

    std::vector<float> vertices;
    std::vector<float> colors;
    std::vector<float> uvs;
    std::vector<float> normals;

    for (auto vert : repackList){
        vertices.push_back(vert.pos.x);
        vertices.push_back(vert.pos.y);
        vertices.push_back(vert.pos.z);

        colors.push_back(vert.col.x);
        colors.push_back(vert.col.y);
        colors.push_back(vert.col.z);

        uvs.push_back(vert.uvs.x);
        uvs.push_back(vert.uvs.y);

        normals.push_back(vert.nml.x);
        normals.push_back(vert.nml.y);
        normals.push_back(vert.nml.z);
    }

    return {r.is3d, vertices, colors, uvs, normals, indices, r.shaderProgram, r.texture, r.testDepth};
}

std::vector<std::string> splitStr(std::string in, char del){
    std::string store;

    std::stringstream stream(in);

    std::vector<std::string> v;

    while (getline(stream, store, del)) {
        v.push_back(store);
    }

    std::vector<std::string> v2;
    for (auto a : v){
        if (a != ""){
            v2.push_back(a);
        }
    }

    return v2;
}

std::vector<Renderable> loadObj(std::string path, unsigned int shaderProgram){
    std::ifstream fileStream(path);
    if (!fileStream.good()){
        return {{}};
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    std::string fileContents = buffer.str();
    fileContents.append("\n"); // janky fix for OBJs ending without newline not loading last face

    std::vector<Renderable> renderableList;

    std::string currentLine;
    Renderable currentRenderable;
    currentRenderable.is3d = true;
    currentRenderable.enabled = true;
    currentRenderable.testDepth = true;
    currentRenderable.texture = 0;
    currentRenderable.shaderProgram = shaderProgram;

    // Stupid solution to everything starting at 1.
    std::vector<glm::vec3> modelVertices = {{0, 0, 0}};
    std::vector<glm::vec2> modelUVs = {{0, 0}};
    std::vector<glm::vec3> modelNormals = {{0, 1, 0}};

    for (char currentChar : fileContents){
        if (currentChar != '\n') currentLine += currentChar;
        else {
            std::vector<std::string> split = splitStr(currentLine, ' ');
            currentLine = "";

            if (split.size() == 0 || split[0] == "#") {
                continue; // This line of the file is a comment
            }

            // God I wish a switch/case worked here...
            if        (split[0] == "v")  {
                // vertex position
                modelVertices.push_back({std::stof(split[1]), std::stof(split[2]), std::stof(split[3])});
            } else if (split[0] == "vt") {
                // vertex texture coordinate
                modelUVs.push_back({std::stof(split[1]), std::stof(split[2])});
            } else if (split[0] == "vn") {
                // vertex normal
                modelNormals.push_back({std::stof(split[1]), std::stof(split[2]), std::stof(split[3])});
            } else if (split[0] == "f")  {
                // face
                for (int i = 1; i <= 3; i++){
                    // for each index combo
                    if (split[i].find("//") != std::string::npos){
                        // formatted as v//n
                        std::vector<std::string> vertexSplit = splitStr(split[i], '/');
                        unsigned int vertexIndex = std::stoi(vertexSplit[0]);
                        unsigned int normalIndex = std::stoi(vertexSplit[1]);

                        currentRenderable.indices.push_back(currentRenderable.vertices.size()/3);

                        currentRenderable.vertices.push_back(modelVertices[vertexIndex].x);
                        currentRenderable.vertices.push_back(modelVertices[vertexIndex].y);
                        currentRenderable.vertices.push_back(modelVertices[vertexIndex].z);

                        currentRenderable.uvs.push_back(modelUVs[0].x);
                        currentRenderable.uvs.push_back(modelUVs[0].y);

                        currentRenderable.normals.push_back(modelNormals[normalIndex].x);
                        currentRenderable.normals.push_back(modelNormals[normalIndex].y);
                        currentRenderable.normals.push_back(modelNormals[normalIndex].z);

                        currentRenderable.colors.push_back(1);
                        currentRenderable.colors.push_back(1);
                        currentRenderable.colors.push_back(1);
                    } else if (split[i].find('/') != std::string::npos) {
                        // formatted as either v/t or v/t/n
                        std::vector<std::string> vertexSplit = splitStr(split[i], '/');
                        unsigned int vertexIndex = std::stoi(vertexSplit[0]);
                        unsigned int texcrdIndex = std::stoi(vertexSplit[1]);
                        unsigned int normalIndex = 0;
                        if (vertexSplit.size() == 3) {
                            normalIndex = std::stoi(vertexSplit[2]);
                        }


                        currentRenderable.indices.push_back(currentRenderable.vertices.size()/3);

                        currentRenderable.vertices.push_back(modelVertices[vertexIndex].x);
                        currentRenderable.vertices.push_back(modelVertices[vertexIndex].y);
                        currentRenderable.vertices.push_back(modelVertices[vertexIndex].z);

                        currentRenderable.uvs.push_back(modelUVs[texcrdIndex].x);
                        currentRenderable.uvs.push_back(modelUVs[texcrdIndex].y);

                        currentRenderable.normals.push_back(modelNormals[normalIndex].x);
                        currentRenderable.normals.push_back(modelNormals[normalIndex].y);
                        currentRenderable.normals.push_back(modelNormals[normalIndex].z);

                        currentRenderable.colors.push_back(1);
                        currentRenderable.colors.push_back(1);
                        currentRenderable.colors.push_back(1);
                    } else {
                        // formatted as v
                        unsigned int index = std::stoi(split[i]);

                        currentRenderable.indices.push_back(currentRenderable.vertices.size()/3);

                        currentRenderable.vertices.push_back(modelVertices[index].x);
                        currentRenderable.vertices.push_back(modelVertices[index].y);
                        currentRenderable.vertices.push_back(modelVertices[index].z);

                        currentRenderable.uvs.push_back(modelUVs[0].x);
                        currentRenderable.uvs.push_back(modelUVs[0].y);

                        currentRenderable.normals.push_back(modelNormals[0].x);
                        currentRenderable.normals.push_back(modelNormals[0].y);
                        currentRenderable.normals.push_back(modelNormals[0].z);

                        currentRenderable.colors.push_back(1);
                        currentRenderable.colors.push_back(1);
                        currentRenderable.colors.push_back(1);
                    }
                }
            } else if (split[0] == "s"){
                // smooth shaded enabled (ignore this case)
            } else if (split[0] == "mtllib"){
                // load material (ignore this case)
            } else if (split[0] == "usemtl" || split[0] == "o"){
                // use specific material
                if (!currentRenderable.vertices.empty()) {
                    renderableList.push_back(repackRenderable(currentRenderable));
                }
                currentRenderable = Renderable();
                currentRenderable.is3d = true;
                currentRenderable.enabled = true;
                currentRenderable.testDepth = true;
                currentRenderable.texture = getTexture(split[1]);
                if (!textureExists(split[1])){
                    std::cout << "\"" << split[1] << "\" not found in texture map! Please load the associated texture to the map under the name of the .mtl file." << std::endl;
                }
                currentRenderable.shaderProgram = shaderProgram;
            }
            else if (split[0] == "vp" || split[0] == "l"){
                std::cerr << "OBJ model at " << path << " uses unsupported parameter space vertex or line element! Cancelling load." << std::endl;
                return {};
            } else {
                std::cerr << "Unrecognized token \"" << split[0] << "\""<< std::endl;
            }
        }
    }
    renderableList.push_back(repackRenderable(currentRenderable));
    return renderableList;
}