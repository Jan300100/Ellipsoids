#include "Structs.hlsl"
#include "Helpers.hlsl"


//input
ConstantBuffer<AppData> gAppData : register(b0);
StructuredBuffer<Rasterizer> gRasterizers : register(t0);
StructuredBuffer<OutQuadric> gQuadrics : register(t1);

//output
RWTexture2D<float4> gColorBuffer : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);

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
    uint2 virtualTextureLeftTop = GetScreenLeftTop(rasterizerIndex, gAppData.tileDimensions * uint(sqrt((float) gAppData.numRasterizers) + 1.0f), gAppData.tileDimensions);
    float ndcLeft = ScreenToNDC(screenLeftTop.x, gAppData.windowDimensions.x);
    float ndcTop = -ScreenToNDC(screenLeftTop.y, gAppData.windowDimensions.y);
    float ndcBottom = ndcTop - gAppData.tileDimensions.y * delta.y;
    float ndcRight = ndcLeft + gAppData.tileDimensions.x * delta.x;
    
    //PER QUADRIC
    for (uint qIdx = rasterizer.rasterizerIdx * gAppData.quadricsPerRasterizer; qIdx < rasterizer.rasterizerIdx * gAppData.quadricsPerRasterizer + rasterizer.numQuadrics; qIdx++)
    {
        OutQuadric q = gQuadrics[qIdx];
        
        float bottom = clamp(q.yRange.x, ndcTop, ndcBottom);
        float yNdc = bottom + scanline * delta.y;
        
        //scan every quadric
        float xMin, xMax;
        float a = q.transform[0][0];
        float b = 2 * (q.transform[0][1] * yNdc + q.transform[0][2]);
        float c = q.transform[1][1] * yNdc * yNdc + 2 * yNdc * q.transform[1][2] + q.transform[2][2];
        //solve for xmin,xmax + early out if th quadric is not on this scanline
        if (!SolveQuadratic(a, b, c, xMin, xMax) || (xMin > xMax) || (xMin > ndcRight || xMax < ndcLeft))
                continue;
        
        xMin = clamp(xMin, ndcLeft, ndcRight);
        xMax = clamp(xMax, ndcLeft, ndcRight);
        
        //some of these steps can be moved to rasterizer initialization
        uint2 screenPixel = NDCToScreen(float2(xMin, yNdc), gAppData.windowDimensions);        

        gColorBuffer[uint2(0, 0)] = 1;
        for (float xNdc = xMin; xNdc < xMax; xNdc += delta.x, screenPixel.x++)
        {
            uint2 localPixel = screenPixel - screenLeftTop;
            gColorBuffer[(virtualTextureLeftTop + localPixel).xy] = float4(q.color, 1);
        }

        
        /*typical forward difference calculation for zs^2,
        for the normal vector n, 
        and for the definition space point p.*/
        //float zSqrd = (a * xMin + b) * xMin + c; //value at xs
        //float dzSqrd = 2 * a * xMin + a + b; //first difference at xs
        //float ddzSqrd = 2 * a; //secnd difference at xs
        //float3 Nx, Nz, NxXplusN0; // only need 3 dim
        //float3 Px, Pz, PxXplusP0; // only need 3 dim
        //float4x4 tsd = mul(q.shearToProj, gAppData.viewProjInv);
        //[unroll]
        //for (int i = 0; i != 3; ++i)
        //{
        //    Nz[i] = q.normalGenerator[2][i];
        //    NxXplusN0[i] = q.normalGenerator[0][i] * xMin + q.normalGenerator[1][i] * yValue + q.normalGenerator[3][i];
        //    Nx[i] = q.normalGenerator[0][i];
            
        //    Pz[i] = tsd[2][i];
        //    PxXplusP0[i] = tsd[0][i] * xMin + tsd[1][i] * yValue + tsd[3][i];
        //    Px[i] = tsd[0][i];
        //}
        
        //for (float x = xMin; x < xMax; x += delta.x)
        //{
        //    float zs = sqrt(zSqrd);
        //    zSqrd += dzSqrd;
        //    dzSqrd += ddzSqrd;
        //    float3 worldPos = zs * Pz + PxXplusP0;
        //    PxXplusP0 += Px;
        //    float3 normal = zs * Nz + NxXplusN0;
        //    NxXplusN0 += Nx;
            
        //    float depth = 0;
        //    [unroll]
        //    for (int i = 0; i < 4; i++)
        //    {
        //        depth += q.shearToProj[2][i] * zs;
        //    }

        //    if (depth < gDepthBuffer[pixel])
        //    { //fill GBuffers
        //        gDepthBuffer[pixel] = depth;
        //    }
            
        //    //increment pixel
        //    pixel.x++;            
        //}
    }
}
