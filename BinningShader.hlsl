#include "Structs.hlsl"
#include "Helpers.hlsl"

//input
ConstantBuffer<AppData> gAppData : register(b0);
ConstantBuffer<MeshData> gMeshData : register(b1);
StructuredBuffer<OutQuadric> gQuadrics : register(t0);
StructuredBuffer<ScreenTile> gScreenTiles : register(t1);

//output
RWStructuredBuffer<uint> gQuadricIndices : register(u0);
RWStructuredBuffer<Tile> gRelevantTiles : register(u1);


[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= gMeshData.numQuadrics)
        return;
    
    OutQuadric input = gQuadrics[DTid.x];
    
        //////// set tiles
    uint2 numTiles = uint2((gAppData.windowDimensions / gAppData.tileDimensions) + 1);
    uint2 start = NDCToScreen(float2(input.xRange.x, input.yRange.y), gAppData.windowDimensions) / gAppData.tileDimensions;
    start = uint2(max(start.x, 0), max(start.y, 0));
    uint2 end = NDCToScreen(float2(input.xRange.y, input.yRange.x), gAppData.windowDimensions) / gAppData.tileDimensions;
    end = uint2(max(end.x, numTiles.x), max(end.y, numTiles.y));

    for (uint x = start.x; x < end.x; x++)
    {
        for (uint y = start.y; y < end.y; y++)
        {
            uint index = y * numTiles.x + x;
            
            uint tileIdx = gScreenTiles[index].tileIndex;
            uint currentCtr;
            uint quadricIndex;
            bool foundIndex = false;
            do
            {
                currentCtr = gRelevantTiles[tileIdx].quadricCtr;
                if (currentCtr < gRelevantTiles[tileIdx].quadricsReserved)
                {
                    InterlockedCompareExchange(gRelevantTiles[tileIdx].quadricCtr, currentCtr, currentCtr + 1, quadricIndex);
                    if (currentCtr == quadricIndex)
                    {
                        foundIndex = true;
                    }
                }
                else
                {
                    tileIdx = gRelevantTiles[tileIdx].nextTileIndex;
                }
                
            } while (!foundIndex);
            
            gQuadricIndices[gRelevantTiles[tileIdx].quadricStartIndex + quadricIndex] = DTid.x;
        }
    }
}