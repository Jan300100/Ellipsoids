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
    
    uint2 scrP = uint2(UINT_MAX, (screenLeftTop.y + scanline));
    //PER QUADRIC
    for (uint qIdx = rasterizerIndex * gAppData.quadricsPerRasterizer; qIdx < rasterizerIndex * gAppData.quadricsPerRasterizer + rasterizer.numQuadrics; qIdx++)
    {
        OutQuadric q = gRasterizerQBuffer[qIdx];

        if (scrP.y > NDCToScreen(-q.yRange.x, gAppData.windowDimensions.y)
            || scrP.y < NDCToScreen(-q.yRange.y, gAppData.windowDimensions.y))
        {
            continue;
        }
        
        float3 pos;
        uint2 rBufPixel;
        float4 projPos;
        for (uint x = 0; x < gAppData.tileDimensions.x; x++)
        {
            rBufPixel = virtualTextureLeftTop + uint2(x, scanline);
            pos = float3(ScreenToNDC(screenLeftTop + uint2(x, scanline), gAppData.windowDimensions), 1);
            pos.z = mul(mul(float3(pos), q.transform), float3(pos));
            if (pos.z > 0) //this pixels covers the ellipsoid
            {
                if (gAppData.reverseDepth)
                {
                    pos.z = -sqrt(pos.z);
                    projPos = mul(float4(pos, 1), q.shearToProj);
                    if (projPos.z < 0.0 || projPos.z > 1.0f)
                        continue;
                    float depth = projPos.z;
                    if (gRDepthBuffer[rBufPixel.xy] <= depth)
                    {
                        gRIBuffer[rBufPixel.xy] = qIdx;
                        gRDepthBuffer[rBufPixel.xy] = depth;
                    }
                }
                else
                {
                    pos.z = sqrt(pos.z);
                    projPos = mul(float4(pos, 1), q.shearToProj);
                    if (projPos.z < 0.0 || projPos.z > 1.0f)
                        continue;
                    float depth = projPos.z;
                    if (gRDepthBuffer[rBufPixel.xy] > depth)
                    {
                        gRIBuffer[rBufPixel.xy] = qIdx;
                        gRDepthBuffer[rBufPixel.xy] = depth;
                    }
                }
            }
        }
    }
}
