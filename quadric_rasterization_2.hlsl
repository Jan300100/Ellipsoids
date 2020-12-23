//*************
//RASTERIZATION
//*************

struct Data
{
    float4x4 viewProjInv;
    float4x4 viewInv;
    float4x4 projInv;
    float2 windowDimensions;
    float3 lightDirection;
};

struct OutQuadric
{
    float4x4 shearToProj;
    float4x4 normalGenerator;
    float3x3 transform;
    float3 color;
    //bounding box
    float2 yRange;
    float2 xRange;
    //
    uint2 startPixel;
    uint numPatches;
    uint numHorizontalPatches;
};
//input
ConstantBuffer<Data> gData : register(b0);
StructuredBuffer<OutQuadric> gQuadrics : register(t0);

//output
RWTexture2D<float4> gOutputBuffer : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);

//groupshared
groupshared OutQuadric s_Quadric;
groupshared uint2 s_startPixel;

#define patchSize 32
[numthreads(patchSize, patchSize, 1)]
void Rasterization(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.xy = (uint2)0)//only 1 thread does this
    {
        bool found = false;
        OutQuadric q = gQuadrics[0];
        uint currentPatch = 0;
        for (uint index = 0;;index++)
        {
            q = gQuadrics[index];
            currentPatch += q.numPatches;

            if (currentPatch > DTid.z)
            {
                currentPatch -= q.numPatches;
                q = gQuadrics[--index];
                break;
            }

        }
        uint localPatch = DTid.z - currentPatch;
        int2 pixelStart = q.startPixel + (int2(localPatch % q.numHorizontalPatches, localPatch / q.numHorizontalPatches.x) * patchSize);
    
        s_Quadric = q;
        s_startPixel = pixelStart;
    }
    GroupMemoryBarrierWithGroupSync();

    uint2 pixel = (s_startPixel + DTid.xy);
    
    float3 pos = float3
    (
    (pixel.x / gData.windowDimensions.x - 0.5f) * 2.0f,
    -(pixel.y / gData.windowDimensions.y - 0.5f) * 2.0f
    , 1
    );
    
    bool outsideBB = (pos.y < s_Quadric.yRange.x || pos.y > s_Quadric.yRange.y
         || pos.x < s_Quadric.xRange.x || pos.x > s_Quadric.xRange.y);
    if (!outsideBB)
    {
        pos.z = mul(mul(float3(pos), s_Quadric.transform), float3(pos));
        if (pos.z > 0) //this pixels covers the ellipsoid
        {
            //float depth = mul(float4(pos, 1), shearToProj);
            pos.z = sqrt(pos.z);
            float4 projPos = mul(float4(pos, 1), s_Quadric.shearToProj);
            float4 defPos = mul(projPos, transpose(gData.viewProjInv));
            if (defPos.w < 0)
                return; //this is behind us / we dont want to render stuff thats behind the camera
        
            float depth = projPos.z;
            if (gDepthBuffer[pixel] > depth)
            {
                gDepthBuffer[pixel] = depth;

            //
                float3 normal = -mul(float4(pos, 1), s_Quadric.normalGenerator).xyz;
                normal = normalize(normal);
                float3 lDir = normalize(gData.lightDirection.xyz); //make sure its normalized
                float lambertDot = dot(normal, lDir);
                float4 diffuse = float4(s_Quadric.color, 1) * lambertDot;
                gOutputBuffer[pixel] = diffuse;
            }
        }
    }
}