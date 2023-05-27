#include "Base.hlsl"

bool Project(InQuadric input, uint instanceIdx, out OutQuadric output);

[numthreads(BATCH_SIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x >= gNumQuadrics)
        return;
    
    OutQuadric projected;
    bool success = Project(gQuadricsIn[DTid.x], DTid.z, projected);
    if (success)
    {
        // store // maybe we can store it inline, saving registers and potentially more bandwidth
        // 1 big store would be terrible for memory, outquadrics are quite big
    }
}

bool Project(InQuadric input, uint instanceIdx, out OutQuadric output)
{
    output = (OutQuadric)0;
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
            return false;
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
            return false;
        }
        
        output.yRange = float2(yMin, yMax);
    }
    else
    {
        return false;
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
                return false;
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
        return false;
    }
    
    //calc bounding box (store w projected)
    uint2 numTiles = GetNrTiles(gAppData.windowDimensions, gAppData.tileDimensions);
    output.bbStart = NDCToScreen(float2(output.xRange.x, output.yRange.y), gAppData.windowDimensions) / gAppData.tileDimensions;
    output.bbEnd = NDCToScreen(float2(output.xRange.y, output.yRange.x), gAppData.windowDimensions) / gAppData.tileDimensions;

    // cull if offscreen
    if (output.bbStart.x >= numTiles.x || output.bbEnd.x < 0 || output.bbStart.y >= numTiles.y || output.bbEnd.y < 0)
    {
        return false;
    }
    
    // clamp to screen (-1 because these are indices)
    output.bbStart.x = clamp(output.bbStart.x, 0, numTiles.x - 1);
    output.bbStart.y = clamp(output.bbStart.y, 0, numTiles.y - 1);
    output.bbEnd.x = clamp(output.bbEnd.x, 0, numTiles.x - 1);
    output.bbEnd.y = clamp(output.bbEnd.y, 0, numTiles.y - 1);
    
    //normal generator
    output.normalGenerator = mul(mul(tsd, world), transpose(gAppData.viewInv));
    output.color = input.color.rgb;

    return true;
}