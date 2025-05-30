struct ParticleData
{
    float3 Pos;
    float3 Vel;
    float Age;
    float Lifetime;
};

cbuffer BindlessRenderResources : register(b0)
{
    int BindlessIndexParticlesBufferInput;
    int BindlessIndexParticlesBufferOutput;
}

[numthreads(20, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[BindlessIndexParticlesBufferInput];
    RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[BindlessIndexParticlesBufferOutput];

    
    ParticleData Data = particlesBufferIn[DTid.x];
    
    Data.Pos += float3(0, 0, -1);
    Data.Age += 0.016f;
    
    particlesBufferOut[DTid.x] = Data;
}