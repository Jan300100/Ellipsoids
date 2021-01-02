//**********
//PROJECTION
//**********

struct Data
{
    float4x4 viewProjInv;
    float4x4 viewInv;
    float4x4 projInv;
    float2 windowDimensions;
    float3 lightDirection;
};

struct MeshData
{
    float4x4 transform;
    uint totalPatches;
};

struct InQuadric
{
    float4x4 transformed;
    float4 color;
};

struct OutQuadric
{
    float4x4 shearToProj;
    float4x4 covariantTensor;
    float3x3 transform;
    float3 color;
    /////
    uint2 startPixel;
    uint startPatch;
    uint numPatches;
    uint numHorizontalPatches;
};


//input
ConstantBuffer<Data> gData : register(b0);
StructuredBuffer<InQuadric> gInput : register(t0);

//output
RWStructuredBuffer<OutQuadric> gOutput : register(u0);
RWStructuredBuffer<MeshData> gMeshData : register(u1);

uint2 ToPixel(float2 ndc);
void BoundingBox(float3x3 quadric, out float2 min, out float2 max);
bool SolveQuadratic(float a, float b, float c, out float min, out float max);

#define patchSize 32
#define boundingBoxRange 1
#define mesh gMeshData[0]

//transforms ellipsoids to the correct space
[numthreads(32, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    InQuadric input = gInput[id.x];
    OutQuadric output = (OutQuadric) 0;
    output.color = input.color.rgb;
    
    float4x4 world = input.transformed;
    world = mul(mul(transpose(mesh.transform), input.transformed), mesh.transform);
    
    float4x4 projected = mul(mul(transpose(gData.viewProjInv), world), gData.viewProjInv);

    float4x4 shearTransform =
    {
        1, 0, - projected[0][2] / projected[2][2], 0,
        0, 1, - projected[1][2] / projected[2][2], 0,
        0, 0, -1, 0,
        0, 0, - projected[3][2] / projected[2][2], 1
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
    output.covariantTensor = mul(tsd, world);
    output.covariantTensor = mul(output.covariantTensor, gData.viewInv);
    output.transform = simplified;
    
    float2 min, max;
    BoundingBox(simplified, min, max);
    output.startPixel = ToPixel(float2(min.x,min.y));
    output.numHorizontalPatches = ceil((max.x - min.x) / patchSize);
    output.numPatches = output.numHorizontalPatches * ceil((max.y - min.y) / patchSize);
    InterlockedAdd(mesh.totalPatches, output.numPatches, output.startPatch);
    //output.startPatch = 0// filled in in next stage
    gOutput[id.x] = output;
}

void BoundingBox(float3x3 quadric, out float2 min, out float2 max)
{
    
    //calc QTilde Inverse elements we need
    float qStar22 = quadric[0][0] * quadric[1][1] - quadric[0][1] * quadric[1][0];
    float qStar12 = quadric[0][1] * quadric[2][0] - quadric[0][0] * quadric[2][1];
    float qStar11 = quadric[0][0] * quadric[2][2] - quadric[0][2] * quadric[2][0];
    float qStar00 = quadric[1][1] * quadric[2][2] - quadric[1][2] * quadric[2][1];
    float qStar02 = quadric[1][0] * quadric[2][1] - quadric[2][0] * quadric[1][1];
    
    //calc a, b, c
    float a = -qStar22;
    float b = 2 * qStar12;
    float c = -qStar11;
    
    if (SolveQuadratic(a, b, c, min.y, max.y))
    {
        if (min.y > max.y)
        {
            max.y = boundingBoxRange;
            min.y = -boundingBoxRange;
        }
    }
    
    //calc a, b, c
    a = -qStar22;
    b = 2 * qStar02;
    c = -qStar00;
    
    if (SolveQuadratic(a, b, c, min.x, max.x))
    {
        if (min.x > max.x)
        {
            max.x = boundingBoxRange;
            min.x = -boundingBoxRange;
        }
    }
}
bool SolveQuadratic(float a, float b, float c, out float min, out float max)
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
            min = -boundingBoxRange;
            max = boundingBoxRange;
            return true;
        }
        float d = sqrt(discr);
        if (a > 0)
        {
            d = -d; //signal that its hyperbolic
        }
        min = -ba - d;
        max = -ba + d;
        return true;
    }
    if (b!=0.0f)
    {
        if (b > 0)
        {
            min = -c / b;
            max = boundingBoxRange;
        }
        else
        {
            min = -boundingBoxRange;
            max = -c / b;
        }
        return true;
    }
    if (c < 0)
        return false;
        
    min = -boundingBoxRange;
    max = boundingBoxRange;
    
    return true;
}
uint2 ToPixel(float2 ndc)
{
    uint2 pixel;
    pixel = (ndc / 2.0f) + 0.5f;
    pixel.x *= gData.windowDimensions.x;
    pixel.y *= -gData.windowDimensions.y;
    return pixel;
}