#define UINT_MAX 0xffffffff

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
    uint showTiles;
    uint reverseDepth;
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
    uint nextRasterizerIdx; //linked list
    uint numQuadrics;
};


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



uint NDCToScreen(float ndc, float dimension)
{
    return (ndc / 2.0f + 0.5f) * dimension;
}

uint2 NDCToScreen(float2 ndc, float2 windowDimensions)
{
    return uint2(NDCToScreen(ndc.x, windowDimensions.x),
     NDCToScreen(-ndc.y, windowDimensions.y));
}

float ScreenToNDC(uint screen, float dimension)
{
    return (screen / dimension - 0.5f) * 2.0f;
}

float2 ScreenToNDC(uint2 screen, float2 windowDimensions)
{
    return float2(ScreenToNDC(screen.x, windowDimensions.x),
     -ScreenToNDC(screen.y, windowDimensions.y));
}

bool SolveQuadratic(float a, float b, float c, out float minValue, out float maxValue)
{
    bool returnValue = false;
    minValue = -1;
    maxValue = 1;
    
    if (a != 0.0f)
    {
        float ba = b / (2 * a);
        float ca = c / a;
        float discr = ba * ba - ca;
        if (discr < 0)
        {
            //nothing
            returnValue = !(c < 0);
        }
        else
        {
            float d = sqrt(discr);
            if (a > 0)
            {
                d = -d; //signal that its hyperbolic
            }
            maxValue = -ba + d;
            minValue = -ba - d;
            returnValue = true;
        }
    }
    else if (b != 0.0f)
    {
        if (b > 0)
        {
            minValue = -c / b;
        }
        else
        {
            maxValue = -c / b;
        }
        returnValue = true;
    }
    else
    {
        returnValue = !(c < 0);
    }
    return returnValue;
}

uint2 GetNrTiles(uint2 windowDimensions, uint2 rasterizerDimensions)
{
    return uint2(windowDimensions.x / rasterizerDimensions.x + (windowDimensions.x % rasterizerDimensions.x > 0)
    , windowDimensions.y / rasterizerDimensions.y + (windowDimensions.y % rasterizerDimensions.y > 0));
}

uint2 GetScreenLeftTop(uint index, uint2 textureDimensions, uint2 rasterizerDimensions)
{
    uint2 screenLeftTop;
    uint nrTilesHorizontal = GetNrTiles(textureDimensions, rasterizerDimensions).x;
    screenLeftTop.x = (index % nrTilesHorizontal) * rasterizerDimensions.x;
    screenLeftTop.y = (index / nrTilesHorizontal) * rasterizerDimensions.y;
    return screenLeftTop;
}



