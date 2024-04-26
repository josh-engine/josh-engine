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

fragment float4 _main(VertexOut in [[stage_in]],
                               texture2d<float> colorTexture [[texture(0)]]) {
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);
    // Sample the texture to obtain a color
    const float4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);
    return colorSample;
}