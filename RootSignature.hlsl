#include "Structs.hlsl"

uint gNumQuadrics : register(b0);
ConstantBuffer<AppData> gAppData : register(b1);

StructuredBuffer<float4x4> gMeshData : register(t0);
StructuredBuffer<InQuadric> gQuadricsIn : register(t1);

RWStructuredBuffer<Rasterizer> gRasterizers : register(u0);
RWStructuredBuffer<ScreenTile> gScreenTiles : register(u1);
RWStructuredBuffer<OutQuadric> gRasterizerQBuffer : register(u2);

//desc heap
RWTexture2D<float4> gBackBuffer : register(u3);
RWTexture2D<uint> gRIBuffer : register(u4);
RWTexture2D<float> gDepthBuffer : register(u5);
RWTexture2D<float> gRDepthBuffer : register(u6);



