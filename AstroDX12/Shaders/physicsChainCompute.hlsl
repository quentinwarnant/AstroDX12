

struct ParticleData
{
   float3 Pos;
   float3 PrevPos;
   float3x3 Rot;
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
    const float3 gravity = float3(0.f, -9.81f, 0.f);
    
    if(!Data.Pinned)
    {
        float3 newPos = Data.Particle.Pos; 
        newPos += (Data.Particle.Pos - Data.Particle.PrevPos) * 0.99f; // velocity + Damping
        newPos += gravity * dt; // Gravity

        ChainElementData ParentData = chainDataBufferIn[Data.ParentIndex];
        
        // TODO: iterate serially over all elements to satisfy constraints
        float3 NodeToParent = ParentData.Particle.Pos - newPos;
        float distanceToParent = length(NodeToParent);
        float lengthCorrection = distanceToParent - Data.RestLength;
        if (abs(lengthCorrection) > 0.0001f)
        {
            float3 correctionDir = normalize(NodeToParent) * lengthCorrection;
            newPos += correctionDir ;
        }
        
        Data.Particle.PrevPos = Data.Particle.Pos;
        Data.Particle.Pos = newPos;
    }
    
    chainDataBufferOut[DTid.x] = Data;
}