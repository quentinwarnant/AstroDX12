

struct ParticleData
{
   float3 Pos;
};

struct ChainElementData
{
    ParticleData Particle;
    int ParentIndex;
    float RestLength;
    bool Pinned;
};


cbuffer BindlessRenderResources : register(b0)
{
    int BindlessIndexChainElementBufferInput;
    int BindlessIndexChainElementBufferOutput;
}

[numthreads(5, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    StructuredBuffer<ChainElementData> chainDataBufferIn = ResourceDescriptorHeap[BindlessIndexChainElementBufferInput];
    RWStructuredBuffer<ChainElementData> chainDataBufferOut = ResourceDescriptorHeap[BindlessIndexChainElementBufferOutput];
    
    ChainElementData Data = chainDataBufferIn[DTid.x];
   
    
    const float dt = 1.f / 60.f; // should be an input arg

    
    chainDataBufferOut[DTid.x] = Data;
}