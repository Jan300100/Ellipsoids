struct AppData
{
    row_major float4x4 viewProjInv;
    row_major float4x4 viewInv;
    row_major float4x4 projInv;
    uint2 windowDimensions;
    float3 lightDirection;
    uint2 tileDimensions;
    uint quadricsPerTile;
};

struct InQuadric
{
    float4x4 transformed;
    float4 color;
};

struct OutQuadric
{
    float4x4 shearToProj;
    float4x4 normalGenerator;
    float3x3 transform;
    float3 color;
    float2 yRange;
    float2 xRange;
};

struct MeshData
{
    row_major float4x4 transform;
    uint numQuadrics;
};

struct ShaderOutput
{
    uint numRelevantTiles;
};

struct ScreenTile
{
    uint tileIndex;
    uint numQuadrics; //counter
};

struct Tile
{
    uint screenTileIndex; //indicates position on the screen
    uint quadricStartIndex;
    uint quadricsReserved; //set by screentile
    uint quadricCtr; //used by quadrics that want to add themselves to this buffer
    uint nextTileIndex; //if this Tile is saturated with quadrics, they should be added here.
};
