#include "Structs.hlsl"
#include "Helpers.hlsl"


//input
ConstantBuffer<AppData> gAppData : register(b0);
StructuredBuffer<Rasterizer> gRasterizers : register(t0);
StructuredBuffer<ScreenTile> gScreenTiles : register(t1); //use to find the first rasterizer in the rasterizerbuffer

//descriptorheap 1
Texture2D<float4> gGBufferColor : register(t2);
Texture2D<float> gGBufferDepth : register(t3);

//output
//descriptorheap 2 
RWTexture2D<float4> gBackBuffer : register(u2);
RWTexture2D<float> gDepthBuffer : register(u3);


[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint screenTileIdx = DTid.z;
    uint2 screenLeftTop = GetScreenLeftTop(screenTileIdx, gAppData.windowDimensions, gAppData.tileDimensions);
    uint2 pixel = screenLeftTop.xy + DTid.xy;
    uint rasterizerIdx = gScreenTiles[screenTileIdx].rasterizerHint;
    while (rasterizerIdx != UINT_MAX)
    {
        uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIdx, gAppData.tileDimensions * uint(sqrt((float) gAppData.numRasterizers) + 1.0f), gAppData.tileDimensions);
        gBackBuffer[pixel.xy] = gGBufferColor[virtualTextureLeftTop.xy + DTid.xy];
        rasterizerIdx = gRasterizers[rasterizerIdx].nextRasterizerIdx;
    }
}