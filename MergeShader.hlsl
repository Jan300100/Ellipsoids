#include "Helpers.hlsl"
#include "RootSignature.hlsl"

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
    gRIBuffer.GetDimensions(virtualDimensions.x, virtualDimensions.y);
    float currentDepth = gDepthBuffer[pixel.xy];
    uint currentIdx = UINT_MAX;
    while (rasterizerIdx < gAppData.numRasterizers)
    {
        uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIdx, virtualDimensions, gAppData.tileDimensions);
        uint2 rBufferPixel = virtualTextureLeftTop.xy + DTid.xy;
        float pixelDepth = gRDepthBuffer[rBufferPixel.xy];
        
        #ifdef REVERSED_DEPTH
        if (currentDepth <= pixelDepth)
        {
            currentDepth = pixelDepth;
            currentIdx = gRIBuffer[rBufferPixel.xy];
        }
        #else
        if (currentDepth > pixelDepth)
        {
            currentDepth = pixelDepth;
            currentIdx = gRIBuffer[rBufferPixel.xy];
        }
        #endif
        rasterizerIdx = gRasterizers[rasterizerIdx].nextRasterizerIdx;
    }
    
#ifdef SHOW_TILES
    if (pixel.x % gAppData.tileDimensions.x == 0)
    {
        gDepthBuffer[pixel.xy] = 0;
        gBackBuffer[pixel.xy] = float4(1, 0, 0, 1);
        return;
    }
    else if (pixel.y % gAppData.tileDimensions.y == 0)
    {
        gDepthBuffer[pixel.xy] = 0;
        gBackBuffer[pixel.xy] = float4(0, 1, 1, 1);
        return;
    }
    else if ((pixel.x + 1) % gAppData.tileDimensions.x == 0)
    {
        gDepthBuffer[pixel.xy] = 0;
        gBackBuffer[pixel.xy] = float4(0, 1, 0, 1);
        return;
    }
    else if ((pixel.y + 1)  % gAppData.tileDimensions.y == 0)
    {
        gDepthBuffer[pixel.xy] = 0;
        gBackBuffer[pixel.xy] = float4(1, 1, 0, 1);
        return;
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
        
        #ifdef REVERSED_DEPTH
        pos.z = -sqrt(pos.z);
        #else
        pos.z = sqrt(pos.z);
        #endif
        float3 normal = -mul(float4(pos, 1), q.normalGenerator).xyz;
        normal = normalize(normal);
        float3 lDir = gAppData.lightDirection.xyz;
        float lambertDot = dot(normal, lDir);
        float4 diffuse = float4(q.color, 1) * lambertDot;
        gDepthBuffer[pixel.xy] = currentDepth;
        gBackBuffer[pixel.xy] = diffuse;
    }
    
}