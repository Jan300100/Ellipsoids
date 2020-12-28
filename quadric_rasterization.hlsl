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
StructuredBuffer<OutQuadric> gInput : register(t0);

//output
RWTexture2D<float4> gOutputBuffer : register(u0);
RWTexture2D<float> gDepthBuffer : register(u1);

#define patchSize 32

[numthreads(patchSize, patchSize, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    
    OutQuadric input = gInput[id.z];
    
    float3 pos = float3
    (
    (id.x / gData.windowDimensions.x - 0.5f) * 2.0f,
    -(id.y / gData.windowDimensions.y - 0.5f) * 2.0f
    , 1
    );
    
    if (pos.y < input.startPixel.y || pos.y > (input.startPixel.y + (input.numPatches / input.numHorizontalPatches) * patchSize)
        || pos.x < input.startPixel.x || pos.x > (input.startPixel.x + input.numHorizontalPatches * patchSize))
    {
        return;
    }
    
    pos.z = mul(mul(float3(pos), input.transform), float3(pos));
    if (pos.z > 0) //this pixels covers the ellipsoid
    {
        //float depth = mul(float4(pos, 1), shearToProj);
        pos.z = sqrt(pos.z);
        float4 projPos = mul(float4(pos, 1), input.shearToProj);
        float4 wPos = mul(projPos, transpose(gData.viewProjInv));
        if (wPos.w < 0)
            return; //this is behind us / we dont want to render stuff thats behind the camera
        
        float depth = projPos.z;
        if (gDepthBuffer[id.xy] > depth)
        {
            gDepthBuffer[id.xy] = depth;

            //
            float3 normal = -mul(float4(pos, 1), input.covariantTensor).xyz;
            normal = normalize(normal);
            float3 lDir = normalize(gData.lightDirection.xyz); //make sure its normalized
            float lambertDot = dot(normal, lDir);
            float4 diffuse = float4(input.color, 1) * lambertDot;
            gOutputBuffer[id.xy] = diffuse;
        }
    }
}

