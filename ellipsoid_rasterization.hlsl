struct Data
{
    float2 windowDimensions;
    float3 lDir;
};

struct ProjectedEllipsoid
{
    float4x4 transform;
    float3 color;
};

//input
ConstantBuffer<ProjectedEllipsoid> gEllipsoid : register(b0);
ConstantBuffer<Data> gData : register(b1);
//output
RWTexture2D<float4> gOutputTexture : register(u0);


[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float3 worldID = float3
    (
    (id.x / gData.windowDimensions.x - 0.5f) * 2.0f,
    -(id.y / gData.windowDimensions.y - 0.5f) * 2.0f
    , 1
    );
    float result = mul(mul(float3(worldID), (float3x3)gEllipsoid.transform),
    float3(worldID));

    gOutputTexture[id.xy] = (result > 0) * float4(gEllipsoid.color,1);
}