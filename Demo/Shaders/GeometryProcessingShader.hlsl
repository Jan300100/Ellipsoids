#include "Base.hlsl"



OutQuadric Project(InQuadric q, uint instanceIdx);
void AddQuadric(uint screenTileIdx, OutQuadric quadric);

[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= gNumQuadrics)
        return;
    //PROJECT
    OutQuadric projected = Project(gQuadricsIn[DTid.x], DTid.z);

    
    //FILL RASTERIZERS
    uint2 numTiles = GetNrTiles(gAppData.windowDimensions, gAppData.tileDimensions);
    uint2 start = NDCToScreen(float2(projected.xRange.x, projected.yRange.y), gAppData.windowDimensions) / gAppData.tileDimensions;
    uint2 end = NDCToScreen(float2(projected.xRange.y, projected.yRange.x), gAppData.windowDimensions) / gAppData.tileDimensions;
    if (start.x >= numTiles.x || end.x < 0 || start.y >= numTiles.y || end.y < 0)
        return;
    
    //-1 because these are indices
    start.x = clamp(start.x, 0, numTiles.x-1);
    start.y = clamp(start.y, 0, numTiles.y-1);
    end.x = clamp(end.x, 0, numTiles.x-1);
    end.y = clamp(end.y, 0, numTiles.y-1);
    
    for (uint x = start.x; x <= end.x; x++)
    {
        for (uint y = start.y; y <= end.y; y++)
        {
            uint screenIdx = y * numTiles.x + x;
            if (screenIdx < numTiles.x * numTiles.y)
            {
                AddQuadric(screenIdx, projected);
            }
        }
    }
}

void AddQuadric(uint screenTileIdx, OutQuadric quadric)
{
    uint numRasterizers = gAppData.numRasterizers;
    uint screenHint = gScreenTiles[screenTileIdx].rasterizerHint;
    uint rIdx = (screenHint < numRasterizers) * screenHint;
    while (rIdx < numRasterizers)
    {
        if (gRasterizers[rIdx].screenTileIdx == UINT_MAX)
        {
            //try claim for this screenTile
            uint original;
            InterlockedCompareExchange(gRasterizers[rIdx].screenTileIdx, UINT_MAX, screenTileIdx, original);
            if (original == UINT_MAX)
            {
                //we claimed the rasterizer, append to linkedlist
                uint llIdx;
                InterlockedCompareExchange(gScreenTiles[screenTileIdx].rasterizerHint, UINT_MAX, rIdx, llIdx);
                while (llIdx < numRasterizers)
                {
                    InterlockedCompareExchange(gRasterizers[llIdx].nextRasterizerIdx, UINT_MAX, rIdx, llIdx);
                }
            }
        }
        else if (gRasterizers[rIdx].screenTileIdx == screenTileIdx)
        {
            uint value = gRasterizers[rIdx].numQuadrics;
            if (value < gAppData.quadricsPerRasterizer)
            {
                //try add to this rasterizer
                uint qIdx;
                InterlockedCompareExchange(gRasterizers[rIdx].numQuadrics, value, value + 1, qIdx);
                if (qIdx == value)
                {
                    //spot secured : add the quadric
                    uint rasterizerOffset = rIdx * gAppData.quadricsPerRasterizer;
                    gRasterizerQBuffer[rasterizerOffset + qIdx] = quadric;
                    rIdx = UINT_MAX;
                }
            }
            else
            {
                uint hint = gRasterizers[rIdx].nextRasterizerIdx;
                rIdx = (hint > rIdx && hint < gAppData.numRasterizers) ? hint : rIdx + 1;
            }
        }
        else
        {
            rIdx++;
        }
    }
}


OutQuadric Project(InQuadric input, uint instanceIdx)
{
    OutQuadric output = (OutQuadric)0;
    float4x4 transform = gMeshData[instanceIdx];
    
    //put the quadric at its's world position
    float4x4 world = mul(mul(transform, input.transformed), transpose(transform));;
    
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
    
    //this one is needed later
    float4x4 tsd = mul(output.shearToProj, gAppData.viewProjInv);
    
    //BOUNDING BOX in NDC
    //calc QTilde Inverse elements we need
    
    //calc a, b, c
    float a = -(output.transform[0][0] * output.transform[1][1] - output.transform[0][1] * output.transform[1][0]);
    float b = 2 * (output.transform[0][1] * output.transform[2][0] - output.transform[0][0] * output.transform[2][1]);
    float c = -(output.transform[0][0] * output.transform[2][2] - output.transform[0][2] * output.transform[2][0]);
    float yMin = 0, yMax = 0;
    
    if (SolveQuadratic(a, b, c, yMin, yMax))
    {
       
        //needs to not be 0, because we need to divide by this value when calculating x
        if (output.transform[0][0] == 0)
        {
            output.yRange = float2(1, -1);    
            return output;
        }
        //calculate x at yMin, so we have a point on the yMin branch 
        //(there is only 1 possible x value, because we are on the edge of the quadric)
        float xm = -(output.transform[0][1] * yMin + output.transform[0][2]) / output.transform[0][0];
        //calculate the W component of this point in definition space : 
            //if < 0 : not a valid point, keep the yMax branch.
            //else : valid point, keep yMin branch
        float w = xm * tsd[0][3] + yMin * tsd[1][3] + tsd[3][3];
        if (yMin > yMax)
        {
            //keep correct branch
            yMin = (w < 0) ? -1 : yMin;
            yMax = (w < 0) ? yMax : 1;
        }
        else if (w < 0)
        {
            output.yRange = float2(1, -1);
            return output;
        }
        
        output.yRange = float2(yMin, yMax);
    }
    else
    {
        output.yRange = float2(1, -1);
        return output;
    }
    
    //calc a, b, c, a is the same as for the y range
    b = 2 * (output.transform[1][0] * output.transform[2][1] - output.transform[2][0] * output.transform[1][1]);
    c = -(output.transform[1][1] * output.transform[2][2] - output.transform[1][2] * output.transform[2][1]);
    float xMin = 0, xMax = 0;
    if (SolveQuadratic(a, b, c, xMin, xMax))
    {
        if (xMin > xMax)
        {
            //same idea here, but we already checked for meshes behind us in the yrange calculation, so this xrange calculation
            //only needs to run when xMin > xMax :)
            //needs to not be 0, because we need to divide by this value when calculating x
            if (output.transform[1][1] == 0)
            {
                output.xRange = float2(1, -1);
                return output;
            }
            //(there is only 1 possible y value, because we are on the edge of the quadric)
            float ym = -(output.transform[1][2] + output.transform[0][1] * xMin) / output.transform[1][1];
            //calculate the W component of this point in definition space : 
                //if < 0 : not a valid point, keep the xMax branch.
                //else : valid point, keep xMin branch
            if (xMin * tsd[0][3] + ym * tsd[1][3] + tsd[3][3] < 0)
                xMin = -1; // keep [-inf,ymax] branch
            else
                xMax = 1; // keep [ymin,+inf] branch
        }
        output.xRange = float2(xMin, xMax);
    }
    else
    {
        output.xRange = float2(1, -1);
        return output;
    }
    
    //normal generator
    output.normalGenerator = mul(mul(tsd, world), transpose(gAppData.viewInv));
    output.color = input.color.rgb;

    return output;
}