#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

struct ParticleData
{
    float3 Pos;
    float3 Vel;
};

cbuffer BindlessRenderResources : register(b0)
{
    int indexParticleInputBuffer;
    int indexParticleOutputBuffer;
    int indexPressureGridInputIndex;
    int indexPressureGridOutputIndex;
    //int velocitygridOutIndex;
}


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    Texture3D<float4> pressureGridIn = ResourceDescriptorHeap[indexPressureGridInputIndex];
    RWTexture3D<float4> pressureGridOut = ResourceDescriptorHeap[indexPressureGridOutputIndex];
    
    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[indexParticleInputBuffer];
    RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[indexParticleOutputBuffer];
    
    
    int linearID = DTid.x + (DTid.y * 32);
    ParticleData p = particlesBufferIn[linearID];
    p.Vel += float3(0.f, -98.1f, 0.f) * (1.f / 60.f);
    p.Pos += p.Vel * (1.f / 60.f);
    
    // A bit of nonesense, just for testing
    float someVal = p.Pos.x;
    if (pressureGridIn[DTid].x > 0.f)
    {
        someVal += 1.f;
    }
    
    particlesBufferOut[linearID] = p;
    
    
    pressureGridOut[DTid] = pressureGridIn[DTid] + float4(someVal, 0.f, 0.f, 0.f);

}