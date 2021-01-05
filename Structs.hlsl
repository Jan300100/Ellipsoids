struct AppData
{
    row_major float4x4 viewProjInv;
    row_major float4x4 viewInv;
    row_major float4x4 projInv;
    uint2 windowDimensions;
    float3 lightDirection;
    uint2 tileDimensions;
    uint quadricsPerRasterizer;
    uint numRasterizers;
};

struct InQuadric
{
    float4x4 transformed;
    float4 color;
};

struct OutQuadric
{
    float4x4 shearToProj;
    float4x4 normalGenerator;
    float3x3 transform;
    float3 color;
    float2 yRange;
    float2 xRange;
};

struct ScreenTile
{
    uint rasterizerHint;
};

struct Rasterizer
{
    uint screenTileIdx;
    uint rasterizerIdx;
    uint nextRasterizerIdx; //linked list
    uint numQuadrics;
};
