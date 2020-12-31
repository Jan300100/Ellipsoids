#include "Structs.hlsl"
#include "Helpers.hlsl"

//input
ConstantBuffer<AppData> gAppData : register(b0);
ConstantBuffer<MeshData> gMeshData : register(b1);

StructuredBuffer<InQuadric> gQuadricsIn : register(t0);
//output
RWStructuredBuffer<OutQuadric> gQuadricsOut : register(u0);
RWStructuredBuffer<ScreenTile> gScreenTiles : register(u1);

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= gMeshData.numQuadrics)
        return;
    
    InQuadric input = gQuadricsIn[DTid.x];
    OutQuadric output = (OutQuadric) 0;
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
    float4x4 tsd = mul(output.shearToProj, gAppData.viewProjInv);
    output.normalGenerator = mul(mul(tsd, world), transpose(gAppData.viewInv));
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
    gQuadricsOut[DTid.x] = output;

    //////// set tiles
    uint2 numTiles = uint2((gAppData.windowDimensions / gAppData.tileDimensions) + 1);
    uint2 start = NDCToScreen(float2(output.xRange.x, output.yRange.y), gAppData.windowDimensions) / gAppData.tileDimensions;
    start = uint2(max(start.x, 0), max(start.y, 0));
    uint2 end = NDCToScreen(float2(output.xRange.y, output.yRange.x), gAppData.windowDimensions) / gAppData.tileDimensions;
    end = uint2(min(end.x, numTiles.x), min(end.y, numTiles.y));
    for (uint x = start.x; x < end.x; x++)
    {
        for (uint y = start.y; y < end.y; y++)
        {
            uint index = y * numTiles.x + x;
            InterlockedAdd(gScreenTiles[index].numQuadrics, 1);
        }
    }
}
