struct AppData
{
    row_major float4x4 viewProjInv;
    row_major float4x4 viewInv;
    row_major float4x4 projInv;
    uint2 windowDimensions;
    float3 lightDirection;
    uint2 tileDimensions;
    uint quadricsPerRasterizer;
    uint numRasterizers;
};

struct InQuadric
{
    float4x4 transformed;
    float4 color;
};

struct OutQuadric
{
    float4x4 shearToProj; //Tsp
    float4x4 normalGenerator; //M
    float3x3 transform; //Qtilde from the articles
    float3 color;
    float2 yRange; //boundingrange of the ellipsoid
    float2 xRange;
};

bool SolveQuadratic(float a, float b, float c, out float minValue, out float maxValue)
{
    bool returnValue = false;
    minValue = -1;
    maxValue = 1;
    
    if (a != 0.0f)
    {
        float ba = b / (2 * a);
        float ca = c / a;
        float discr = ba * ba - ca;
        if (discr < 0)
        {
            //nothing
            returnValue = !(c < 0);
        }
        else
        {
            float d = sqrt(discr);
            if (a > 0)
            {
                d = -d; //signal that its hyperbolic
            }
            maxValue = -ba + d;
            minValue = -ba - d;
            returnValue = true;
        }
    }
    else if (b != 0.0f)
    {
        if (b > 0)
        {
            minValue = -c / b;
        }
        else
        {
            maxValue = -c / b;
        }
        returnValue = true;
    }
    else
    {
        returnValue = !(c < 0);
    }
    return returnValue;
}


float ScreenToNDC(uint screen, float dimension)
{
    return (screen / dimension - 0.5f) * 2.0f;
}

float2 ScreenToNDC(uint2 screen, float2 windowDimensions)
{
    return float2(ScreenToNDC(screen.x, windowDimensions.x),
     -ScreenToNDC(screen.y, windowDimensions.y));
}

////
//GLOBAL VARS
ConstantBuffer<AppData> gAppData : register(b1);
StructuredBuffer<float4x4> gMeshData : register(t0); //transform of the ellipsoids
/////

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
    
    //THIS IS THE QTILDE from the articles
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


void Shade(uint2 pixel, OutQuadric q)
{
    //check if pixel covered by Q
    float2 pixelNDC = ScreenToNDC(pixel, gAppData.windowDimensions);
    float3 pos = float3(pixelNDC.x, pixelNDC.y, 1);
    pos.z = mul(mul(float3(pos), q.transform), float3(pos));
    if (pos.z > 0)
    {
        //covered by Q
        pos.z = sqrt(pos.z);
        float4 projPos = mul(float4(pos, 1), q.shearToProj);
        if (projPos.z < 0.0 || projPos.z > 1.0f)
            continue;
        float depth = projPos.z;
        if (gDepthBuffer[pixel.xy] > depth)
        {
            //shading
            float3 normal = -mul(float4(pos, 1), q.normalGenerator).xyz;
            normal = normalize(normal);
            float3 lDir = gAppData.lightDirection.xyz;
            float lambertDot = dot(normal, lDir);
            float4 diffuse = float4(q.color, 1) * lambertDot;
            gColorBuffer[pixel.xy] = diffuse;
            gDepthBuffer[pixel.xy] = depth;
        }
    }
}