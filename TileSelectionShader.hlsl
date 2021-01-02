#include "Structs.hlsl"
#include "Helpers.hlsl"

struct Counters
{
    uint tileCtr;
    uint quadricCtr;
};

//Input
ConstantBuffer<AppData> gAppData : register(b0);

//output
RWStructuredBuffer<ScreenTile> gScreenTiles : register(u0);
RWStructuredBuffer<Tile> gRelevantTiles : register(u1);
RWStructuredBuffer<Counters> gCounters : register(u2);

#define Counters gCounters[0]

Tile ReserveTile(ScreenTile screenTile, out uint tileBufferIndex);

[numthreads(32, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    ScreenTile tile = gScreenTiles[DTid.x];
    if (tile.numQuadrics == 0)
        return;

    uint previousTile = 0xffffffff; //max
    uint index;
    Tile output;
    
    while (tile.numQuadrics > gAppData.quadricsPerTile)
    {
        tile.numQuadrics -= gAppData.quadricsPerTile;
        output = ReserveTile(tile, index);
        output.screenTileIndex = DTid.x;
        output.nextTileIndex = previousTile;
        //gRelevantTiles[index] = output;
        previousTile = index;
    }
    
    output = ReserveTile(tile, index);
    output.screenTileIndex = DTid.x;
    output.nextTileIndex = previousTile;
    gRelevantTiles[index] = output;
    
    //setup link
    gScreenTiles[DTid.x].tileIndex = index;
}

Tile ReserveTile(ScreenTile screenTile, out uint tileBufferIndex)
{
    uint numQuadrics = min(screenTile.numQuadrics, gAppData.quadricsPerTile);
    
    //TileBufferIndex   
    uint value = Counters.tileCtr;
    do
    {
        value = Counters.tileCtr;
        InterlockedCompareExchange(Counters.tileCtr, value, value + 1, tileBufferIndex);
    }
    while (tileBufferIndex != value);
    
    //QuadricStartIndex
    value = Counters.quadricCtr;
    uint quadricStartIndex;
    do
    {
        value = Counters.quadricCtr;
        InterlockedCompareExchange(Counters.quadricCtr, value, value + numQuadrics, quadricStartIndex);
    }
    while (quadricStartIndex != value);
    
    Tile output = (Tile)0;
    output.quadricsReserved = numQuadrics;
    output.quadricStartIndex = quadricStartIndex;
    return output;
}