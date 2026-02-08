#define THREAD_GROUP_SIZE_X 8
#define THREAD_GROUP_SIZE_Y 8
#define THREAD_GROUP_SIZE_Z 8

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
    
    int debugDrawLineVertexBufferIndex;
    int debugDrawLineCounterBufferIndex;
}

struct DebugLineVertex
{
    float3 Pos;
    int ColorIndex;
};


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
    
    
    //if( DTid.x < 2 && DTid.y == 0 && DTid.z == 0)
    {
        RWStructuredBuffer<DebugLineVertex> debugDrawLineVertexBuffer = ResourceDescriptorHeap[debugDrawLineVertexBufferIndex];
        RWByteAddressBuffer debugDrawLineCounterBuffer = ResourceDescriptorHeap[debugDrawLineCounterBufferIndex];
        
        uint cellIndex;
        debugDrawLineCounterBuffer.InterlockedAdd(0, 12, cellIndex); // 12 edges per cube
        if (cellIndex < 10000000) // MAX_DEBUG_LINES_PER_FRAME
        {
            uint vertexIndex = cellIndex * 2;
            
            // BLB (bottom, left, back)
            float3 BLBCorner = cellPos - gridCellHalfSize;
            float3 BLFCorner = cellPos + float3(-gridCellHalfSize.x, -gridCellHalfSize.y, gridCellHalfSize.z);
            float3 TLFCorner = cellPos + float3(-gridCellHalfSize.x, gridCellHalfSize.y, gridCellHalfSize.z);
            float3 TLBCorner = cellPos + float3(-gridCellHalfSize.x, gridCellHalfSize.y, -gridCellHalfSize.z);

            float3 BRBCorner = cellPos + float3(gridCellHalfSize.x, -gridCellHalfSize.y, -gridCellHalfSize.z);
            float3 BRFCorner = cellPos + float3(gridCellHalfSize.x, -gridCellHalfSize.y, gridCellHalfSize.z);
            float3 TRFCorner = cellPos + gridCellHalfSize;
            float3 TRBCorner = cellPos + float3(gridCellHalfSize.x, gridCellHalfSize.y, -gridCellHalfSize.z);
            
            // Left face
            debugDrawLineVertexBuffer[vertexIndex].Pos = BLBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BLFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex+=2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TLBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BLBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            // Right face
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BRBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            
            // connection between faces
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BLBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = 0;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = 1;
        }
    }
    
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