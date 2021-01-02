#include "Helpers.hlsl"
#include "Structs.hlsl"


//input
ConstantBuffer<AppData> gAppData : register(b0);
StructuredBuffer<Rasterizer> gRasterizers : register(t0);
StructuredBuffer<ScreenTile> gScreenTiles : register(t1); //use to find the first rasterizer in the rasterizerbuffer

//descriptorheap 1
Texture2D<float4> gGBufferColor : register(t2);
Texture2D<float> gGBufferDepth : register(t3);

//output
//descriptorheap 2 
RWTexture2D<float4> gBackBuffer : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);


[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint screenTileIdx = DTid.z;
    uint2 screenLeftTop = GetScreenLeftTop(screenTileIdx, gAppData.windowDimensions, gAppData.tileDimensions);
    uint2 pixel = screenLeftTop.xy + DTid.xy;
    if (pixel.x > gAppData.windowDimensions.x 
        || pixel.y > gAppData.windowDimensions.y 
        || DTid.x > gAppData.tileDimensions.x 
        || DTid.y > gAppData.tileDimensions.y)
        return;

    uint rasterizerIdx = gScreenTiles[screenTileIdx].rasterizerHint;
    uint2 virtualDimensions = mul(uint(ceil(sqrt((float) gAppData.numRasterizers))), gAppData.tileDimensions);
    while (rasterizerIdx < gAppData.numRasterizers)
    {
        uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIdx, virtualDimensions, gAppData.tileDimensions);
        uint2 GBufferPixel = virtualTextureLeftTop.xy + DTid.xy;
        float pixelDepth = gGBufferDepth[GBufferPixel.xy];
        if (gDepthBuffer[pixel.xy] > pixelDepth)
        {
            gDepthBuffer[pixel.xy] = pixelDepth;
            gBackBuffer[pixel.xy] = gGBufferColor[GBufferPixel.xy];
        }
        rasterizerIdx = gRasterizers[rasterizerIdx].nextRasterizerIdx;
    }
}