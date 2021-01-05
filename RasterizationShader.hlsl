#include "Helpers.hlsl"
#include "RootSignature.hlsl"




[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint scanline = DTid.x;
    if (scanline >= gAppData.tileDimensions.y)
        return;
    
    //rasterizer INFO
    uint rasterizerIndex = DTid.y;
    Rasterizer rasterizer = gRasterizers[rasterizerIndex];
    float2 delta = float2(2.0f / gAppData.windowDimensions.x, 2.0f / gAppData.windowDimensions.y); //width and height of 1 pixel in NDC
    uint2 screenLeftTop = GetScreenLeftTop(rasterizer.screenTileIdx, gAppData.windowDimensions, gAppData.tileDimensions);
    
    uint2 virtualDimensions;
    gRIBuffer.GetDimensions(virtualDimensions.x, virtualDimensions.y);
    uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIndex, virtualDimensions, gAppData.tileDimensions);
    float ndcLeft = ScreenToNDC(screenLeftTop.x, gAppData.windowDimensions.x);
    float ndcTop = -ScreenToNDC(screenLeftTop.y, gAppData.windowDimensions.y);
    float ndcBottom = ndcTop - (gAppData.tileDimensions.y) * delta.y;
    float ndcRight = ndcLeft + (gAppData.tileDimensions.x) * delta.x;
    
    //PER QUADRIC
    for (uint qIdx = rasterizer.rasterizerIdx * gAppData.quadricsPerRasterizer; qIdx < rasterizer.rasterizerIdx * gAppData.quadricsPerRasterizer + rasterizer.numQuadrics; qIdx++)
    {
        OutQuadric q = gRasterizerQBuffer[qIdx];

        if ((screenLeftTop.y + scanline) > NDCToScreen(-q.yRange.x, gAppData.windowDimensions.y)
            || (screenLeftTop.y + scanline) < NDCToScreen(-q.yRange.y, gAppData.windowDimensions.y))
        {
            continue;
        }
        
        for (uint x = 0; x < gAppData.tileDimensions.x; x++)
        {
            uint2 pixel = virtualTextureLeftTop + uint2(x, scanline);
            float3 pos = float3(ScreenToNDC(screenLeftTop + uint2(x, scanline), gAppData.windowDimensions), 1);
            pos.z = mul(mul(float3(pos), q.transform), float3(pos));
            if (pos.z > 0) //this pixels covers the ellipsoid
            {
                pos.z = sqrt(pos.z);
                float4 projPos = mul(float4(pos, 1), q.shearToProj);
                if (gRDepthBuffer[pixel.xy] > projPos.z)
                {
                    gRIBuffer[pixel.xy] = qIdx;
                    gRDepthBuffer[pixel.xy] = projPos.z;
                }
            }
        }
    }
}
