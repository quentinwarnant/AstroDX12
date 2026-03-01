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

    int indexVelocityGridInputIndex;
    int indexVelocityGridOutputIndex;
    int debugDrawLineVertexBufferIndex;
    int debugDrawLineCounterBufferIndex;

    uint3 pressureGridResolution;
    uint pad0;

    uint3 velocityGridResolution;
    uint pad1;
    
    uint3 gridPosition;
    uint pad2;
    
    int3 pressureGridExtents;
    int particleCount;
}

struct DebugLineVertex
{
    float3 Pos;
    uint ColorIndex;
};

void DebugDrawCellEdges(float3 cellPos, float3 gridCellHalfSize, bool3 isEdgeCell, float pressure)
{
    RWStructuredBuffer<DebugLineVertex> debugDrawLineVertexBuffer = ResourceDescriptorHeap[debugDrawLineVertexBufferIndex];
    RWByteAddressBuffer debugDrawLineCounterBuffer = ResourceDescriptorHeap[debugDrawLineCounterBufferIndex];
        
    int newEdgeCount = 3;
    if (isEdgeCell.x) // There will be a bit of overlap on cube corners, but that's fine
    {
        newEdgeCount += 4;
    }
    if (isEdgeCell.y)
    {
        newEdgeCount += 4;
    }
    if (isEdgeCell.z)
    {
        newEdgeCount += 4;
    }


    
    uint cellIndex;
    debugDrawLineCounterBuffer.InterlockedAdd(0, newEdgeCount, cellIndex); 
    if (cellIndex < 100000000) // MAX_DEBUG_LINES_PER_FRAME
    {
        uint vertexIndex = cellIndex * 2;
            
        // BLB (bottom, left, back)
        const float3 BLBCorner = cellPos - gridCellHalfSize;
        const float3 BLFCorner = cellPos + float3(-gridCellHalfSize.x, -gridCellHalfSize.y, gridCellHalfSize.z);
        const float3 TLFCorner = cellPos + float3(-gridCellHalfSize.x, gridCellHalfSize.y, gridCellHalfSize.z);
        const float3 TLBCorner = cellPos + float3(-gridCellHalfSize.x, gridCellHalfSize.y, -gridCellHalfSize.z);
         
        const float3 BRBCorner = cellPos + float3(gridCellHalfSize.x, -gridCellHalfSize.y, -gridCellHalfSize.z);
        const float3 BRFCorner = cellPos + float3(gridCellHalfSize.x, -gridCellHalfSize.y, gridCellHalfSize.z);
        const float3 TRFCorner = cellPos + gridCellHalfSize;
        const float3 TRBCorner = cellPos + float3(gridCellHalfSize.x, gridCellHalfSize.y, -gridCellHalfSize.z);
            
        // Only draw edges leaving from bottom left back corner to avoid double drawing edges between cells
        // We will fix the outer sides with special cases.
        
        uint colorIndex = pressure > 0 ? 1 : 0;
        // Left face
        debugDrawLineVertexBuffer[vertexIndex].Pos = BLBCorner;
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BLFCorner;
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
            
        vertexIndex += 2;
        debugDrawLineVertexBuffer[vertexIndex].Pos = BLBCorner;
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TLBCorner;
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
            
        vertexIndex += 2;
        debugDrawLineVertexBuffer[vertexIndex].Pos = BLBCorner;
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRBCorner;
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
        
        
        if (isEdgeCell.x)
        {
            debugDrawLineCounterBuffer.InterlockedAdd(0, 4, cellIndex);
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BRBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
                
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;

            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        }
        
        
        if (isEdgeCell.y)
        {
            debugDrawLineCounterBuffer.InterlockedAdd(0, 4, cellIndex);
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
                
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;

            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRBCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TLBCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        }
        
        if (isEdgeCell.z)
        {
            debugDrawLineCounterBuffer.InterlockedAdd(0, 4, cellIndex);
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
                
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TLFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;

            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = TRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
            
            vertexIndex += 2;
            debugDrawLineVertexBuffer[vertexIndex].Pos = BRFCorner;
            debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
            debugDrawLineVertexBuffer[vertexIndex + 1].Pos = BLFCorner;
            debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        }
    }
}


void DebugDrawCellBoundaryCenter(float3 cellPos, float3 gridCellHalfSize)
{
    RWStructuredBuffer<DebugLineVertex> debugDrawLineVertexBuffer = ResourceDescriptorHeap[debugDrawLineVertexBufferIndex];
    RWByteAddressBuffer debugDrawLineCounterBuffer = ResourceDescriptorHeap[debugDrawLineCounterBufferIndex];
    
    uint cellIndex;
    uint amountOfLinesDrawn = 6;
    debugDrawLineCounterBuffer.InterlockedAdd(0, amountOfLinesDrawn, cellIndex);
    if (cellIndex < 100000000) // MAX_DEBUG_LINES_PER_FRAME
    {
            
        // BLB (bottom, left, back)
        const float3 BLBCorner = cellPos - gridCellHalfSize;
        const float3 BLFCorner = cellPos + float3(-gridCellHalfSize.x, -gridCellHalfSize.y, gridCellHalfSize.z);
        const float3 TLFCorner = cellPos + float3(-gridCellHalfSize.x, gridCellHalfSize.y, gridCellHalfSize.z);
        const float3 TLBCorner = cellPos + float3(-gridCellHalfSize.x, gridCellHalfSize.y, -gridCellHalfSize.z);
         
        const float3 BRBCorner = cellPos + float3(gridCellHalfSize.x, -gridCellHalfSize.y, -gridCellHalfSize.z);
        const float3 BRFCorner = cellPos + float3(gridCellHalfSize.x, -gridCellHalfSize.y, gridCellHalfSize.z);
        const float3 TRFCorner = cellPos + gridCellHalfSize;
        const float3 TRBCorner = cellPos + float3(gridCellHalfSize.x, gridCellHalfSize.y, -gridCellHalfSize.z);
            
        uint vertexIndex = cellIndex * 2;
        // Left face
        uint colorIndex = 4;
        debugDrawLineVertexBuffer[vertexIndex].Pos = cellPos - float3(gridCellHalfSize.x, -0.1f, 0);
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = cellPos - float3(gridCellHalfSize.x, 0.1f, 0);
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
        // Right face
        vertexIndex += 2;
        colorIndex = 5;
        debugDrawLineVertexBuffer[vertexIndex].Pos = cellPos + float3(gridCellHalfSize.x, 0, -0.1f);
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = cellPos + float3(gridCellHalfSize.x, 0, 0.1f);
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
        // Front face
        vertexIndex += 2;
        colorIndex = 6;
        debugDrawLineVertexBuffer[vertexIndex].Pos = cellPos - float3(-0.1f, 0, gridCellHalfSize.z);
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = cellPos - float3(0.1f, 0, gridCellHalfSize.z);
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
         // Back face
        vertexIndex += 2;
        colorIndex = 7;
        debugDrawLineVertexBuffer[vertexIndex].Pos = cellPos + float3(0, -0.1f, gridCellHalfSize.z);
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = cellPos + float3(0, 0.1f, gridCellHalfSize.z);
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
        // Top face
        vertexIndex += 2;
        colorIndex = 8;
        debugDrawLineVertexBuffer[vertexIndex].Pos = cellPos + float3(-0.1f, gridCellHalfSize.y, 0);
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = cellPos + float3(0.1f, gridCellHalfSize.y, 0);
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
         // Bottom face
        vertexIndex += 2;
        colorIndex = 0;
        debugDrawLineVertexBuffer[vertexIndex].Pos = cellPos - float3(0, gridCellHalfSize.y, -0.1f);
        debugDrawLineVertexBuffer[vertexIndex].ColorIndex = colorIndex;
        debugDrawLineVertexBuffer[vertexIndex + 1].Pos = cellPos - float3(0, gridCellHalfSize.y, 0.1f);
        debugDrawLineVertexBuffer[vertexIndex + 1].ColorIndex = colorIndex;
        
          
    }
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, THREAD_GROUP_SIZE_Z)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    Texture3D<float> pressureGridIn = ResourceDescriptorHeap[indexPressureGridInputIndex];
    RWTexture3D<float4> pressureGridOut = ResourceDescriptorHeap[indexPressureGridOutputIndex];
    
    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[indexParticleInputBuffer];
    RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[indexParticleOutputBuffer];
    
    float3 extents = float3(pressureGridExtents);
    int3 presGridRes = float3(pressureGridResolution);
    float3 gridPos = float3(gridPosition); // origin is left-botom-back of the grid in world space
    const float3 gridCellSize = extents / presGridRes;
    const float3 gridCellHalfSize = gridCellSize * 0.5f;
    const float3 cellPos = gridPos + (DTid * gridCellSize) + gridCellHalfSize; // centered pos
    
    uint3 pressureGridCellIndex = min(DTid, pressureGridResolution - uint3(1,1,1)); // CLAMP pressure index as there's 1 fewer than velocity grid
    
    float pressure = pressureGridIn[pressureGridCellIndex];
    
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
        pressure = particleCountInCell;
    }
    else
    {
        pressure = 0.f;
    }
    
    pressureGridOut[pressureGridCellIndex] = pressure;
    
    
    
    // Debugging
    {
        bool3 isEdgeCell = bool3(
        DTid.x == velocityGridResolution.x -1,
        DTid.y == velocityGridResolution.y - 1,
        DTid.z == velocityGridResolution.z - 1);
    
        DebugDrawCellEdges(cellPos, gridCellHalfSize, isEdgeCell, pressure);
        DebugDrawCellBoundaryCenter(cellPos, gridCellHalfSize);
    }
    
    
    i = 0;
    for (; i < particleCount; ++i)
    {
        float3 particlePos = particlesBufferIn[i].Pos;
        if (abs(particlePos.x - cellPos.x) < gridCellHalfSize.x &&
            abs(particlePos.y - cellPos.y) < gridCellHalfSize.y &&
            abs(particlePos.z - cellPos.z) < gridCellHalfSize.z)
        {
            // Cell velocity
            particlesBufferOut[i].Vel += accumulatedVel * (1.f / 60.f);
            // Gravity
            particlesBufferOut[i].Vel += float3(0.f, -98.1f, 0.f) * (1.f / 60.f);
            
            // Update position
            particlesBufferOut[i].Pos += particlesBufferOut[i].Vel * (1.f / 60.f);
            
            // Clamp to bounds
            if (particlesBufferOut[i].Pos.x < gridPos.x)
            {
                particlesBufferOut[i].Pos.x = gridPos.x;
                particlesBufferOut[i].Vel.x *= -0.5f;
            }
            else if (particlesBufferOut[i].Pos.x > gridPos.x + pressureGridExtents.x)
            {
                particlesBufferOut[i].Pos.x = gridPos.x + pressureGridExtents.x;
                particlesBufferOut[i].Vel.x *= -0.5f;
            }
            
            if (particlesBufferOut[i].Pos.y < gridPos.y)
            {
                particlesBufferOut[i].Pos.y = gridPos.y;
                particlesBufferOut[i].Vel.y *= -0.5f;
            }
            else if (particlesBufferOut[i].Pos.y > gridPos.y + pressureGridExtents.y)
            {
                particlesBufferOut[i].Pos.y = gridPos.y + pressureGridExtents.y;
                particlesBufferOut[i].Vel.y *= -0.5f;
            }
            
            if (particlesBufferOut[i].Pos.z < gridPos.z)
            {
                particlesBufferOut[i].Pos.z = gridPos.z;
                particlesBufferOut[i].Vel.z *= -0.5f;
            }
            else if (particlesBufferOut[i].Pos.z > gridPos.z + pressureGridExtents.z)
            {
                particlesBufferOut[i].Pos.z = gridPos.z + pressureGridExtents.z;
                particlesBufferOut[i].Vel.z *= -0.5f;
            }
        }
    }
}