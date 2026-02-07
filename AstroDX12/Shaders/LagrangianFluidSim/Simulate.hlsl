#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32
#define THREAD_GROUP_SIZE_Z 1

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
    int3 pressureGridResolution;
    int pad0;
    int3 pressureGridPosition;
    int pad1;
    int3 pressureGridExtents;
    int particleCount;
}


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, THREAD_GROUP_SIZE_Z)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    Texture3D<float4> pressureGridIn = ResourceDescriptorHeap[indexPressureGridInputIndex];
    RWTexture3D<float4> pressureGridOut = ResourceDescriptorHeap[indexPressureGridOutputIndex];
    
    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[indexParticleInputBuffer];
    RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[indexParticleOutputBuffer];
    
    float3 extents = float3(pressureGridExtents);
    int3 gridRes = float3(pressureGridResolution);
    float3 gridPos = float3(pressureGridPosition);
    const float3 gridCellSize = extents / gridRes;
    const float3 gridCellHalfSize = gridCellSize * 0.5f;
    const float3 cellPos = gridPos + (DTid * gridCellSize) + gridCellHalfSize; // centered pos
    float4 cellData = pressureGridIn[DTid];
    
    
    float3 accumulatedVel = float3(0.f, 0.f, 0.f);
    int particleCountInCell = 0;
    int i = 0;
    for (; i < particleCount; ++i)
    {
        float3 particlePos = particlesBufferIn[i].Pos;
        if (abs(particlePos.x - cellPos.x) < gridCellHalfSize.x &&
            abs(particlePos.y - cellPos.y) < gridCellHalfSize.y &&
            abs(particlePos.z - cellPos.z) < gridCellHalfSize.z)
        {
            accumulatedVel += particlesBufferIn[i].Vel;
            particleCountInCell++;
        }
    }
    
    if (particleCountInCell > 0)
    {
        accumulatedVel /= particleCountInCell;
        cellData.xyz = accumulatedVel;
    }
    else
    {
        cellData.xyz = float3(0.f, 0.f, 0.f);
    }
    cellData.w = particleCountInCell;
    
    pressureGridOut[DTid] = cellData;
    
    i = 0;
    for (; i < particleCount; ++i)
    {
        float3 particlePos = particlesBufferIn[i].Pos;
        if (abs(particlePos.x - cellPos.x) < gridCellHalfSize.x &&
            abs(particlePos.y - cellPos.y) < gridCellHalfSize.y &&
            abs(particlePos.z - cellPos.z) < gridCellHalfSize.z)
        {
            // Cell velocity
            particlesBufferOut[i].Vel += cellData.xyz * (1.f / 60.f);
            // Gravity
            particlesBufferOut[i].Vel += float3(0.f, -98.1f, 0.f) * (1.f / 60.f);
            
            // Update position
            particlesBufferOut[i].Pos += particlesBufferOut[i].Vel * (1.f / 60.f);
            
            // Clamp to bounds
            if (particlesBufferOut[i].Pos.x < pressureGridPosition.x)
            {
                particlesBufferOut[i].Pos.x = pressureGridPosition.x;
                particlesBufferOut[i].Vel.x *= -0.5f;
            }
            else if (particlesBufferOut[i].Pos.x > pressureGridPosition.x + pressureGridExtents.x)
            {
                particlesBufferOut[i].Pos.x = pressureGridPosition.x + pressureGridExtents.x;
                particlesBufferOut[i].Vel.x *= -0.5f;
            }
            
            if (particlesBufferOut[i].Pos.y < pressureGridPosition.y)
            {
                particlesBufferOut[i].Pos.y = pressureGridPosition.y;
                particlesBufferOut[i].Vel.y *= -0.5f;
            }
            else if (particlesBufferOut[i].Pos.y > pressureGridPosition.y + pressureGridExtents.y)
            {
                particlesBufferOut[i].Pos.y = pressureGridPosition.y + pressureGridExtents.y;
                particlesBufferOut[i].Vel.y *= -0.5f;
            }
            
            if (particlesBufferOut[i].Pos.z < pressureGridPosition.z)
            {
                particlesBufferOut[i].Pos.z = pressureGridPosition.z;
                particlesBufferOut[i].Vel.z *= -0.5f;
            }
            else if (particlesBufferOut[i].Pos.z > pressureGridPosition.z + pressureGridExtents.z)
            {
                particlesBufferOut[i].Pos.z = pressureGridPosition.z + pressureGridExtents.z;
                particlesBufferOut[i].Vel.z *= -0.5f;
            }

        }
    }
}