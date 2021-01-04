#include "Helpers.hlsl"
#include "RootSignature.hlsl"

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
    uint2 virtualDimensions;
    gGBufferColor.GetDimensions(virtualDimensions.x, virtualDimensions.y);
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