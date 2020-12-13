struct Data
{
    float4 windowDimensions;
    float4 lDir;
};

struct ProjectedEllipsoid
{
    float4x4 transform;
    float4x4 normalGenerator;
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
    //float3 pos = float3
    //(
    //(id.x / gData.windowDimensions.x - 0.5f) * 2.0f,
    //-(id.y / gData.windowDimensions.y - 0.5f) * 2.0f
    //, 1
    //);
    
    float3 pos = float3
    (
    (id.x / gData.windowDimensions.x - 0.5f) * 2.0f,
    -(id.y / gData.windowDimensions.y - 0.5f) * 2.0f
    , 1
    );
    
    pos.z = mul(mul(float3(pos), (float3x3) gEllipsoid.transform), float3(pos));
    if (pos.z > 0) //this pixels covers the ellipsoid
    {
        pos.z = sqrt(pos.z);
        float4 posS = float4(pos, 1);
        
        float3 normal = -mul(gEllipsoid.normalGenerator, posS).xyz;
        normal = normalize(normal);
        float3 lDir = gData.lDir.xyz; //make sure its normalized
        float lambertDot = dot(normal, lDir);
        gOutputTexture[id.xy] = float4(gEllipsoid.color, 1) * (lambertDot);
    }
    

}