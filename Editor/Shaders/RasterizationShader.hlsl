#include "Base.hlsl"

[numthreads(32, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint scanline = DTid.x;
    if (scanline >= gAppData.tileDimensions.y)
        return;
    
    //rasterizer INFO
    uint rasterizerIndex = DTid.y;
    Rasterizer rasterizer = gRasterizers[rasterizerIndex];
    float2 delta = float2(2.0f / gAppData.windowDimensions.x, 2.0f / gAppData.windowDimensions.y); //width and height of 1 pixel in NDC
    uint2 screenLeftTop = GetScreenLeftTop(rasterizer.screenTileIdx, gAppData.windowDimensions, gAppData.tileDimensions);
    
    RWTexture2D<uint> rIBuffer = ResourceDescriptorHeap[gAppData.RasterIBufferIdx];
    RWTexture2D<float> rDepthBuffer = ResourceDescriptorHeap[gAppData.RasterDepthBufferIdx];

    uint2 virtualDimensions;
    rIBuffer.GetDimensions(virtualDimensions.x, virtualDimensions.y);
    uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIndex, virtualDimensions, gAppData.tileDimensions);
    
    
    float posy = -ScreenToNDC(screenLeftTop.y + scanline, gAppData.windowDimensions.y);
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

        float3 pos = float3(0, posy, 1.0f);
        uint2 rBufPixel;
        float4 projPos;
        
        for (uint x = 0; x < gAppData.tileDimensions.x; x++)
        {
            rBufPixel = virtualTextureLeftTop + uint2(x, scanline);
            pos.x = ScreenToNDC(screenLeftTop.x + x, gAppData.windowDimensions.x);
            float zVal = mul(mul(float3(pos), q.transform), float3(pos));
            if (zVal > 0) //this pixels covers the ellipsoid
            {
                zVal = sqrt(zVal);
#if REVERSE_DEPTH
                zVal *= -1;
#endif
                projPos = mul(float4(pos.x, pos.y, zVal, 1), q.shearToProj);
                if (projPos.z < 0.0 || projPos.z > 1.0f)
                    continue;
                
                float depth = projPos.z;
                
#if REVERSE_DEPTH
                if (rDepthBuffer[rBufPixel.xy] <= depth)
#else
                if (rDepthBuffer[rBufPixel.xy] > depth)
#endif
                {
                    rIBuffer[rBufPixel.xy] = qIdx;
                    rDepthBuffer[rBufPixel.xy] = depth;
                }
            }
        }
    }
}
