struct Data
{
    float4x4 viewProjInv;
    float4x4 viewInv;
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

//transforms ellipsoids to the correct space
[numthreads(32, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    InputQuadric input = gInput[id.x];
    ProjectedQuadric output = (ProjectedQuadric)0;
    output.color = input.color.rgb;
    
    float4x4 world = input.transformed;
    world = mul(mul(transpose(gMeshData.transform), input.transformed),gMeshData.transform);
    
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
    
    output.normalGenerator = mul(shearTransform, transpose(gData.viewProjInv));
    output.normalGenerator = mul(output.normalGenerator, world);
    output.normalGenerator = mul(output.normalGenerator, gData.viewInv);
    
    output.transform = simplified;
    gOutput[id.x] = output;
}