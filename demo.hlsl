StructuredBuffer<float> gData : register(t0);
RWStructuredBuffer<float> gOut : register(u0);
RWTexture2D<float4> gOutputTexture : register(u1);

//[numthreads(10, 1, 1)] //dimensions of the thread group
//void main(uint3 DTid : SV_GroupThreadID
//        , uint3 GTid : SV_GroupID)
//{
//    gOut[(GTid.x * 10) + DTid.x] = gData[(GTid.x * 10) + DTid.x] + 5;
//}
//===== SV_DispatchthreadId does the same thing: unique id for each thread
//[numthreads(32, 1, 1)] //dimensions of the thread group
//void main(uint3 DTid : SV_DispatchThreadID)
//{
//    gOut[DTid.x] = gData[DTid.x] + 5;
//}

//USE SV_GroupThreadID to index thread local storage
//USE SV_GroupID to index threadgroup local storage

[numthreads(32, 32, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    int denom = 1;
    float value = gData[id.x];
    
    int range = 5;
    for (int i = 0; i < range; i++)
    {
        if (id.x > i)
        {
            denom += 1;
            value += gData[id.x - (i + 1)];
        }
    }
    value /= denom;
   
    gOutputTexture[id.xy] = float4(1.0, 0.71, 0.29, 1) * value;
    gOut[id.x] = gData[id.x] + 5;
}