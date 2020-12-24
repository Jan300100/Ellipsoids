
struct MeshData
{
    uint numQuadrics;
};

ConstantBuffer<MeshData> gData : register(b0);
StructuredBuffer<uint> gBuffer : register(t0); //sorted

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint target = DTid.z;
    bool found = false;
    uint step = ceil(gData.numQuadrics / 2);
    uint index = floor(gData.numQuadrics / 2);
    
    //unroll(10) : max 1024 quadrics per mesh? should i do this?
    uint maxIterations = log2(gData.numQuadrics) + 1;
    for (uint i = 0; i < maxIterations; i++)
    {
        found = (target >= gBuffer[index]
            && (index + 1 > gData.numQuadrics || target < gBuffer[index + 1]));
        
        //next part will only have effect on the index if not yet found
        step = ceil(step / 2.0f) * (!found);
        int multiplier = int((target > gBuffer[index]) * 2) - 1; // [-1,1] // look up or down in the array
        index += multiplier * step;
    }
    
}


//1.0 project quadrics : output buffer of outQuadrics
//1.1 barrier, output buffer is filled up
//1.1 fill int startPatch for each quadric (scan sum algorithm)
//
//2. shade the patches, find correct quadric through binarysearch : 1024 quadrics in a mesh? : correct one can be found in 10 iterations.