#include "Base.hlsl"

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint screenTileIdx = DTid.z;
    uint rasterizerIdx = gScreenTiles[screenTileIdx].rasterizerHint;
    uint2 screenLeftTop = GetScreenLeftTop(screenTileIdx, gAppData.windowDimensions, gAppData.tileDimensions);
    uint2 pixel = screenLeftTop.xy + DTid.xy;
    if (pixel.x > gAppData.windowDimensions.x 
        || pixel.y > gAppData.windowDimensions.y 
        || DTid.x > gAppData.tileDimensions.x 
        || DTid.y > gAppData.tileDimensions.y
        || rasterizerIdx >= gAppData.numRasterizers)
        return;

    uint2 virtualDimensions;
    
    RWTexture2D<uint> rIBuffer = ResourceDescriptorHeap[gAppData.RasterIBufferIdx];
    RWTexture2D<float> rDepthBuffer = ResourceDescriptorHeap[gAppData.RasterDepthBufferIdx];
    RWTexture2D<float> depthBuffer = ResourceDescriptorHeap[gAppData.depthBufferIdx];
    RWTexture2D<float4> backBuffer = ResourceDescriptorHeap[gAppData.outputBufferIdx];

    rIBuffer.GetDimensions(virtualDimensions.x, virtualDimensions.y);
    float currentDepth = depthBuffer[pixel.xy];
    uint currentIdx = UINT_MAX;

    while (rasterizerIdx < gAppData.numRasterizers)
    {
        uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIdx, virtualDimensions, gAppData.tileDimensions);
        uint2 rBufferPixel = virtualTextureLeftTop.xy + DTid.xy;
        float pixelDepth = rDepthBuffer[rBufferPixel.xy];

#if REVERSE_DEPTH    
        if (currentDepth <= pixelDepth)
#else
        if (currentDepth > pixelDepth)
#endif
        {
            currentDepth = pixelDepth;
            currentIdx = rIBuffer[rBufferPixel.xy];
        }
        
        rasterizerIdx = gRasterizers[rasterizerIdx].nextRasterizerIdx;
    }

#if SHOW_TILES        
    if (gAppData.showTiles)
    {
        if (pixel.x % gAppData.tileDimensions.x == 0)
        {
            depthBuffer[pixel.xy] = 0;
            backBuffer[pixel.xy] = float4(1, 0, 0, 1);
            return;
        }
        else if (pixel.y % gAppData.tileDimensions.y == 0)
        {
            depthBuffer[pixel.xy] = 0;
            backBuffer[pixel.xy] = float4(0, 1, 1, 1);
            return;
        }
        else if ((pixel.x + 1) % gAppData.tileDimensions.x == 0)
        {
            depthBuffer[pixel.xy] = 0;
            backBuffer[pixel.xy] = float4(0, 1, 0, 1);
            return;
        }
        else if ((pixel.y + 1) % gAppData.tileDimensions.y == 0)
        {
            depthBuffer[pixel.xy] = 0;
            backBuffer[pixel.xy] = float4(1, 1, 0, 1);
            return;
        }
    }
#endif
    
    if (currentIdx != UINT_MAX)
    {   
        OutQuadric q = gRasterizerQBuffer[currentIdx];
        
        float2 pixelNDC = ScreenToNDC(pixel, gAppData.windowDimensions);
        //SHADING
        float3 pos = float3(pixelNDC.x ,pixelNDC.y, 1);
        pos.z = mul(mul(float3(pos), q.transform), float3(pos));
        if (pos.z < 0)
            return;
        
        pos.z = sqrt(pos.z);
#if REVERSE_DEPTH
        pos.z *= -1;
#endif
       
        float3 normal = -mul(float4(pos, 1), q.normalGenerator).xyz;
        normal = normalize(normal);
        float3 lDir = gAppData.lightDirection.xyz;
        float lambertDot = dot(normal, lDir);
        float4 diffuse = float4(q.color, 1) * lambertDot;
        depthBuffer[pixel.xy] = currentDepth;
        backBuffer[pixel.xy] = diffuse;
    }
    
}