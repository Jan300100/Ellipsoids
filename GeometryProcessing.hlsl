#include "Structs.hlsl"
#include "Helpers.hlsl"

//input
ConstantBuffer<MeshData> gMeshData : register(b0);

ConstantBuffer<AppData> gAppData : register(b1);

StructuredBuffer<InQuadric> gQuadricsIn : register(t0);

//output
RWStructuredBuffer<OutQuadric> gRasterizerQBuffer : register(u0);
RWStructuredBuffer<Rasterizer> gRasterizers : register(u1);
RWStructuredBuffer<MeshOutputData> gMeshOutput : register(u2);
RWStructuredBuffer<ScreenTile> gScreenTiles : register(u3);

#define MeshOutput gMeshOutput[0]

OutQuadric Project(InQuadric q);
void AddQuadric(uint screenTileIdx, OutQuadric quadric);

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= gMeshData.numQuadrics)
        return;
    //PROJECT
    OutQuadric projected = Project(gQuadricsIn[DTid.x]);
    
    //FILL RASTERIZERS
    uint2 numTiles = uint2((gAppData.windowDimensions / gAppData.tileDimensions) + 1);
    uint2 start = NDCToScreen(float2(projected.xRange.x, projected.yRange.y), gAppData.windowDimensions) / gAppData.tileDimensions;
    uint2 end = NDCToScreen(float2(projected.xRange.y, projected.yRange.x), gAppData.windowDimensions) / gAppData.tileDimensions;
    
    if (start.x > numTiles.x || end.x < 0 || start.y > numTiles.y || end.y < 0)
        return;
    
    start.x = clamp(start.x, 0, numTiles.x);
    start.y = clamp(start.y, 0, numTiles.y);
    end.x = clamp(end.x, 0, numTiles.x);
    end.y = clamp(end.y, 0, numTiles.y);
    
    uint total = (end.x - start.x + 1) * (end.y - start.y + 1);
    InterlockedAdd(MeshOutput.numOutputQuadrics, total);
    for (uint x = start.x; x <= end.x; x++)
    {
        for (uint y = start.y; y <= end.y; y++)
        {

            AddQuadric(y * numTiles.x + x, projected);
            if (MeshOutput.overflowed)
                return;
        }
    }
    
}

void AddQuadric(uint screenTileIdx, OutQuadric quadric)
{
    uint screenHint = gScreenTiles[screenTileIdx].rasterizerHint;
    uint pos = screenHint;
    bool added = false;
    uint previousFull = UINT_MAX;
    while (!added)
    {
        if (pos >= gAppData.numRasterizers)
        {
            //all rasterizers are in use! we can't add the quadric, we would be writing out of bounds
            //but we can act as if we added it, the quadric won't get rendered, but might the next frame.
            added = true;
            MeshOutput.overflowed = true; //tells the cpu this mesh didn't fit in the batch anymore. 
            //next frame, cpu will try again, but will also start with this one on a new batch.
        }
        else if (gRasterizers[pos].screenTileIdx == screenTileIdx)
        {
            uint value = gRasterizers[pos].numQuadrics;
            if (value < gAppData.quadricsPerRasterizer)
            {
                //try add to this rasterizer
                uint qIdx;
                InterlockedCompareExchange(gRasterizers[pos].numQuadrics, value, value + 1, qIdx);
                if (qIdx == value)
                {
                    //spot secured : add the quadric
                    uint rasterizerOffset = pos * gAppData.quadricsPerRasterizer;
                    gRasterizerQBuffer[rasterizerOffset + qIdx] = quadric;
                    added = true;
                }
            }
            else
            {
                uint hint = gRasterizers[pos].nextRasterizerIdx;
                if (hint != UINT_MAX)
                {
                    pos = hint;
                }
                else
                {
                    previousFull = pos;
                    pos++;
                }
            }
        }
        else if (gRasterizers[pos].screenTileIdx == UINT_MAX)
        {
            //try claim for this screenTile
            uint original;
            InterlockedCompareExchange(gRasterizers[pos].screenTileIdx, UINT_MAX, screenTileIdx, original);
            if (original == UINT_MAX)
            {
                //we claimed the raterizer, setup hints for future quadrics
                if (previousFull == UINT_MAX)
                {
                    InterlockedCompareStore(gScreenTiles[screenTileIdx].rasterizerHint, screenHint, pos);
                }
                else
                {
                    InterlockedCompareStore(gRasterizers[previousFull].nextRasterizerIdx, UINT_MAX, pos);
                }
            }
        }
        else
        {
            pos++;
        }
    }
}


OutQuadric Project(InQuadric input)
{
    OutQuadric output = (OutQuadric)0;
    output.color = input.color.rgb;
    
    //put the quadric at its's world position
    float4x4 world = mul(mul(gMeshData.transform, input.transformed), transpose(gMeshData.transform));;
    
    //projection to the screen + perspective
    float4x4 projected = mul(mul(gAppData.viewProjInv, world), transpose(gAppData.viewProjInv));
    
    output.shearToProj = float4x4(
        1, 0, -projected[0][2] / projected[2][2], 0,
        0, 1, -projected[1][2] / projected[2][2], 0,
        0, 0, -1, 0,
        0, 0, -projected[3][2] / projected[2][2], 1
    ); //SHEAR TRANSFORM
    
    float4x4 sheared = mul(mul(output.shearToProj, projected), transpose(output.shearToProj));
    sheared = mul((-1 / projected[2][2]), sheared);
    
    output.transform = float3x3(
        sheared[0][0], sheared[0][1], sheared[0][3],
        sheared[1][0], sheared[1][1], sheared[1][3],
        sheared[3][0], sheared[3][1], sheared[3][3]
    );
    
    //BOUNDING BOX in NDC
    //calc QTilde Inverse elements we need
    
    //calc a, b, c
    float a = -(output.transform[0][0] * output.transform[1][1] - output.transform[0][1] * output.transform[1][0]);
    float b = 2 * (output.transform[0][1] * output.transform[2][0] - output.transform[0][0] * output.transform[2][1]);
    float c = -(output.transform[0][0] * output.transform[2][2] - output.transform[0][2] * output.transform[2][0]);
    float yMin = 0, yMax = 0;
    
    if (SolveQuadratic(a, b, c, yMin, yMax))
    {
        if (yMin > yMax)
        {
            yMax = 1;
            yMin = -1;
        }
        output.yRange = float2(yMin, yMax);
    }
    else
    {
        output.yRange = float2(0, 0);

    }
    
    //calc a, b, c
    b = 2 * (output.transform[1][0] * output.transform[2][1] - output.transform[2][0] * output.transform[1][1]);
    c = -(output.transform[1][1] * output.transform[2][2] - output.transform[1][2] * output.transform[2][1]);
    float xMin = 0, xMax = 0;
    if (SolveQuadratic(a, b, c, xMin, xMax))
    {
        if (xMin > xMax)
        {
            xMax = 1;
            xMin = -1;
        }
        output.xRange = float2(xMin, xMax);
    }
    else
    {
        output.xRange = float2(0, 0);
    }
    
    //normal generator
    float4x4 tsd = mul(output.shearToProj, gAppData.viewProjInv);
    output.normalGenerator = mul(mul(tsd, world), transpose(gAppData.viewInv));
    
    return output;
}