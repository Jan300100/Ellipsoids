struct Data
{
    float4x4 viewProjInv;
    float4x4 viewInv;
    float4x4 projInv;
    float2 windowDimensions;
    float3 lightDirection;
};

struct InputQuadric
{
    float4x4 transformed;
    float4 color;
};

struct ProjectedQuadric
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
    float4x4 transform;
};


//input
ConstantBuffer<Data> gData : register(b0);
ConstantBuffer<MeshData> gMeshData : register(b1);

StructuredBuffer<InputQuadric> gInput : register(t0);
//output
RWStructuredBuffer<ProjectedQuadric> gOutput : register(u0);

bool PosRange(float a, float b, float c, out float yMin, out float yMax);

#define boundingBoxRange 1

//transforms ellipsoids to the correct space
[numthreads(32, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    InputQuadric input = gInput[id.x];
    ProjectedQuadric output = (ProjectedQuadric) 0;
    output.color = input.color.rgb;
    
    float4x4 world = input.transformed;
    world = mul(mul(transpose(gMeshData.transform), input.transformed), gMeshData.transform);
    
    float4x4 projected = mul(mul(transpose(gData.viewProjInv), world), gData.viewProjInv);

    float4x4 shearTransform =
    {
        1, 0, -projected[0][2] / projected[2][2], 0,
        0, 1, -projected[1][2] / projected[2][2], 0,
        0, 0, -1, 0,
        0, 0, -projected[3][2] / projected[2][2], 1
    };
    output.shearToProj = shearTransform;
    
    float4x4 sheared = mul(mul(shearTransform, projected), transpose(shearTransform));
    sheared = mul((-1 / projected[2][2]), sheared);
    float3x3 simplified =
    {
        sheared[0][0], sheared[0][1], sheared[0][3],
        sheared[1][0], sheared[1][1], sheared[1][3],
        sheared[3][0], sheared[3][1], sheared[3][3]
    };
    
    float4x4 tsd = mul(shearTransform, transpose(gData.viewProjInv));
    output.normalGenerator = mul(tsd, world);
    output.normalGenerator = mul(output.normalGenerator, gData.viewInv);
    
    output.transform = simplified;
    //calculate yRange

    //calc QTilde Inverse elements we need
    float qStar22 = simplified[0][0] * simplified[1][1] - simplified[0][1] * simplified[1][0];
    float qStar12 = simplified[0][1] * simplified[2][0] - simplified[0][0] * simplified[2][1];
    float qStar11 = simplified[0][0] * simplified[2][2] - simplified[0][2] * simplified[2][0];
    float qStar00 = simplified[1][1] * simplified[2][2] - simplified[1][2] * simplified[2][1];
    float qStar02 = simplified[1][0] * simplified[2][1] - simplified[2][0] * simplified[1][1];
    
    //calc a, b, c
    float a = -qStar22;
    float b = 2 * qStar12;
    float c = -qStar11;
    float yMin = 0, yMax = 0;
    
    if (PosRange(a, b, c, yMin, yMax))
    {
        if (yMin > yMax)
        {
            //if (simplified[0][0] == 0.0f)
            //{
            //    return; //sphere not visible
            //}
            //float localxMin = -(simplified[0][1] * yMin + simplified[0][2]) / simplified[0][0];
            //if (localxMin * tsd[0][3] + yMin * tsd[1][3] + tsd[3][3] < 0)
            //{
            //    yMin = -boundingBoxRange;
            //}
            //else
            //{
            //    yMax = boundingBoxRange;
            //}
            yMax = boundingBoxRange;
            yMin = -boundingBoxRange;
        }

        output.yRange = float2(yMin, yMax);
    }
    
    //calc a, b, c
    a = -qStar22;
    b = 2 * qStar02;
    c = -qStar00;
    float xMin = 0, xMax = 0;
    
    if (PosRange(a, b, c, xMin, xMax))
    {
        if (xMin > xMax)
        {
            xMax = boundingBoxRange;
            xMin = -boundingBoxRange;
        }
        output.xRange = float2(xMin, xMax);
    }
    
    
    //
    gOutput[id.x] = output;
    
    
    
}


bool PosRange(float a, float b, float c, out float yMin, out float yMax)
{
    
    if (a != 0.0f)
    {
        float ba = b / (2 * a);
        float ca = c / a;
        float discr = ba * ba - ca;
        if (discr < 0)
        {
            //nothing
            if (c < 0)
                return false;
            //entire screen
            yMin = -boundingBoxRange;
            yMax = boundingBoxRange;
            return true;
        }
        float d = sqrt(discr);
        if (a > 0)
        {
            d = -d; //signal that its hyperbolic
        }
        yMax = -ba + d;
        yMin = -ba - d;
        return true;
    }
    if (b!=0.0f)
    {
        if (b > 0)
        {
            yMin = -c / b;
            yMax = boundingBoxRange;
        }
        else
        {
            yMin = -boundingBoxRange;
            yMax = -c / b;
        }
        return true;
    }
    if (c < 0)
        return false;
        
    yMin = -boundingBoxRange;
    yMax = boundingBoxRange;
    return true;
}