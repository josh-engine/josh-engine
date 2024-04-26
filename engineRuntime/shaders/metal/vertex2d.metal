#include <metal_stdlib>
using namespace metal;

struct VertexData {
    float3 position;
    float3 normal;
    float2 textureCoordinate;
};

struct VertexOut {
    float4 position [[position]];
    float4 normal;
    float2 textureCoordinate;
};

struct UBO {
    float4x4 viewMatrix;
    float4x4 _2dProj;
    float4x4 _3dProj;
    float3 cameraPos;
    float3 cameraDir;
    float3 sunDir;
    float3 sunColor;
    float3 ambience;
};

struct PerObject {
    float4x4 model;
    float4x4 normal;
};

vertex VertexOut _main(
            uint vertexID [[vertex_id]],
            constant VertexData* vertexData,
            constant UBO* ubo,
            constant PerObject* push_constants
            ) {
    VertexOut out;
    out.position = (ubo->_2dProj * push_constants->model) * float4(vertexData[vertexID].position, 1.0);
    out.normal = push_constants->normal * float4(vertexData[vertexID].normal, 1.0);
    out.textureCoordinate = vertexData[vertexID].textureCoordinate;
    return out;
}