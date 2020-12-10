struct Data
{
    float4x4 viewProjInv;
};

struct EllipsoidInput
{
    float4x4 transform;
};

struct EllipsoidOutput
{
    float3x3 transform;
    float2 topLeft, bottomRight;
};

//input
ConstantBuffer<Data> gData : register(b0);
StructuredBuffer<EllipsoidInput> gInput : register(t0);
//output
RWStructuredBuffer<EllipsoidOutput> gOutput : register(u0);

//transforms ellipsoids to the correct space
[numthreads(32, 1, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    EllipsoidInput input = gInput[id.x];
    
    float4x4 projected = mul(mul(gData.viewProjInv, input.transform), transpose(gData.viewProjInv));
    float4x4 shearTransform =
    {
        1, 0, - projected[0][2] / projected[2][2], 0,
        0, 1, - projected[1][2] / projected[2][2], 0,
        0, 0, - projected[2][2] / projected[2][2], 0,
        0, 0, - projected[3][2] / projected[2][2], 1
    };
    
    float4x4 sheared = mul(mul(shearTransform, projected), transpose(shearTransform));
    float3x3 simplified =
    {
        sheared[0][0], sheared[0][1], sheared[0][3],
        sheared[1][0], sheared[1][1], sheared[1][3],
        sheared[3][0], sheared[3][1], sheared[3][3]
    };
    gOutput[id.x].transform = simplified;
}