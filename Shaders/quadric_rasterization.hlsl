#include "../Structs.hlsl"
#include "../Helpers.hlsl"

//input
ConstantBuffer<AppData> gData : register(b0);
StructuredBuffer<OutQuadric> gInput : register(t0);

//output
RWTexture2D<float4> gOutputBuffer : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    
    OutQuadric input = gInput[id.z];
   
    uint2 pixel = id.xy + NDCToScreen(float2(input.xRange.x, input.yRange.y), gData.windowDimensions);
    
    float3 pos = float3
    (
    (pixel.x / float(gData.windowDimensions.x) - 0.5f) * 2.0f,
    -(pixel.y / float(gData.windowDimensions.y) - 0.5f) * 2.0f
    , 1
    );
    
    
    if (pos.y < input.yRange.x || pos.y > input.yRange.y
         || pos.x < input.xRange.x || pos.x > input.xRange.y)
    {
        return;
    }

    pos.z = mul(mul(float3(pos), input.transform), float3(pos));
    if (pos.z > 0) //this pixels covers the ellipsoid
    {
        //float depth = mul(float4(pos, 1), shearToProj);
        pos.z = sqrt(pos.z);
        float4 projPos = mul(float4(pos, 1), input.shearToProj);
        float4 defPos = mul(projPos, gData.viewProjInv);
        if (defPos.w < 0)
            return; //this is behind us / we dont want to render stuff thats behind the camera
        
        float depth = projPos.z;
        if (gDepthBuffer[pixel.xy] > depth)
        {

            gDepthBuffer[pixel.xy] = depth;

            //
            float3 normal = -mul(float4(pos, 1), input.normalGenerator).xyz;
            normal = normalize(normal);
            float3 lDir = normalize(gData.lightDirection.xyz); //make sure its normalized
            float lambertDot = dot(normal, lDir);
            float4 diffuse = float4(input.color, 1) * lambertDot;
            gOutputBuffer[pixel.xy] = diffuse;
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

