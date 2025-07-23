
struct ParticleData
{
    float3 Pos;
    float3 Vel;
    float Age;
    float Lifetime;
    float Size;
};

cbuffer BindlessRenderResources : register(b0)
{
    int BindlessIndexParticlesBufferInput;
    int BindlessIndexParticlesBufferOutput;
}

float3 Random(float seed)
{
    return float3(cos(seed * 53.13f) * 8.42f, sin(seed * 13.13f) * 4.92f, tan(seed * 95.83f) * 19.22f);
}

void ResetParticle(inout ParticleData p, uint3 DTid)
{
    p.Age -= p.Lifetime;
    p.Pos = Random(p.Age + DTid.x) * float3(1, 0, 1);
    p.Vel = float3(0.f, 60.f, 0.f) + (normalize(Random(p.Age + DTid.x + 1.234f)) * float3(20.f, 40.f, 20.f));
}

[numthreads(20, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[BindlessIndexParticlesBufferInput];
    RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[BindlessIndexParticlesBufferOutput];
    
    ParticleData Data = particlesBufferIn[DTid.x];
    // Reset
    if (Data.Age > Data.Lifetime)
    {
        ResetParticle(Data, DTid);
    }
    
    const float dt = 1.f / 60.f; // should be an input arg
    
    Data.Vel += float3(0.f, -98.1f, 0.f) * dt;
    Data.Pos += Data.Vel * dt;
    
    if(Data.Pos.y < -2.f)
    {
        Data.Vel.xz *= 0.9f;
        Data.Vel.y *= -1.f;
        Data.Pos.y = -2.f;

    }
    
    Data.Age += 0.016f;
    

    
    particlesBufferOut[DTid.x] = Data;
}