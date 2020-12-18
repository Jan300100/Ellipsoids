struct Data
{
    float4x4 viewProjInv;
    float4x4 viewInv;
    float4x4 projInv;
    float2 windowDimensions;
    float3 lightDirection;
};

struct ProjectedEllipsoid
{
    float4x4 shearToProj;
    float4x4 normalGenerator;
    float3x3 transform;
    float3 color;
    float2 yRange;
};


//input
ConstantBuffer<Data> gData : register(b0);
StructuredBuffer<ProjectedEllipsoid> gInput : register(t0);

//output
RWTexture2D<float4> gOutputBuffer : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    
    ProjectedEllipsoid input = gInput[id.z];
    
    float3 pos = float3
    (
    (id.x / gData.windowDimensions.x - 0.5f) * 2.0f,
    -(id.y / gData.windowDimensions.y - 0.5f) * 2.0f
    , 1
    );
    
    if (pos.y < input.yRange.x|| pos.y > input.yRange.y)
    {
        return;
    }
    
    pos.z = mul(mul(float3(pos), input.transform), float3(pos));
    if (pos.z > 0) //this pixels covers the ellipsoid
    {
        //float depth = mul(float4(pos, 1), shearToProj);
        pos.z = sqrt(pos.z);
        float4 projPos = mul(float4(pos, 1), input.shearToProj);
        float4 defPos = mul(projPos, transpose(gData.viewProjInv));
        if (defPos.w < 0)
            return; //this is behind us / we dont want to render stuff thats behind the camera
        
        float depth = projPos.z;
        if (gDepthBuffer[id.xy] > depth)
        {
            gDepthBuffer[id.xy] = depth;

            //
            float3 normal = -mul(float4(pos, 1), input.normalGenerator).xyz;
            normal = normalize(normal);
            float3 lDir = normalize(gData.lightDirection.xyz); //make sure its normalized
            float lambertDot = dot(normal, lDir);
            float4 diffuse = float4(input.color, 1) * lambertDot;
            gOutputBuffer[id.xy] = diffuse;
        }
    }

    
    //For Antialiasing test ? wouldnt work for intersecting ellipsoids
    //float falloff = -0.0001f;
    //else if (pos.z > falloff)
    //{
    //    float intensity = 1 - (pos.z / falloff);
        
    //    pos.z = sqrt(-pos.z);
    //    float4 posS = float4(pos, 1);
        
    //    float3 normal = -mul(gEllipsoid.normalGenerator, posS).xyz;
    //    normal = normalize(normal);
    //    float3 lDir = gData.lightDirection.xyz; //make sure its normalized
    //    float lambertDot = dot(normal, lDir);
    //    gOutputTexture[id.xy] = gOutputTexture[id.xy] * (1 - intensity) + float4(gEllipsoid.color, 1) * (lambertDot) * intensity;
    //}
}

