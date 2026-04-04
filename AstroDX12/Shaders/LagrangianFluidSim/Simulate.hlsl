#define PRESSUREGRID_THREAD_GROUP_SIZE_X 8
#define PRESSUREGRID_THREAD_GROUP_SIZE_Y 8
#define PRESSUREGRID_THREAD_GROUP_SIZE_Z 8

#define PARTICLES_THREAD_GROUP_SIZE_X 32
#define PARTICLES_THREAD_GROUP_SIZE_Y 1
#define PARTICLES_THREAD_GROUP_SIZE_Z 1


#define MAX_PARTICLES_PER_CELL 8

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

void DebugDrawCellEdges(float3 cellPos, float3 gridCellHalfSize, bool3 isEdgeCell)
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
        
        uint colorIndex = 0;
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

struct ParticleGridData
{
    uint3 baseCellIndexH; // Horizontal
    uint3 baseCellIndexV; // Vertical
    uint3 baseCellIndexD; // Depth
    float3 weightH;
    float3 weightV;
    float3 weightD;
};

ParticleGridData CalculateParticleGridData(float3 particlePos, float3 gridLowerEndCorner, float3 gridCellSize)
{
    ParticleGridData results;

    float3 localPos = particlePos - gridLowerEndCorner;
    
    // We apply an offset of a half cell, to find the bottom left back cell which the particle will splat it's velocity into.
    // the other cells are +1 in each direction & the diagonal.
 
    // Horizontal offset
    float3 cellIndexF = (localPos - float3(gridCellSize.x * .5f, 0.f, 0.f)) / gridCellSize;
    uint3 cellIndex = (uint3) floor(cellIndexF);
    results.baseCellIndexH = cellIndex;
    results.weightH = cellIndexF - cellIndex;
    
    // Vertical offset
    cellIndexF = (localPos - float3(0.f, gridCellSize.y * .5f, 0.f)) / gridCellSize;
    cellIndex = (uint3) floor(cellIndexF);
    results.baseCellIndexV = cellIndex;
    results.weightV = cellIndexF - cellIndex;
    
    // Depth offset
    cellIndexF = (localPos - float3(0.f, 0.f, gridCellSize.z * .5f)) / gridCellSize;
    cellIndex = (uint3) floor(cellIndexF);
    results.baseCellIndexD = cellIndex;
    results.weightD = cellIndexF - cellIndex;
    
    return results;
}

[numthreads(PRESSUREGRID_THREAD_GROUP_SIZE_X, PRESSUREGRID_THREAD_GROUP_SIZE_Y, PRESSUREGRID_THREAD_GROUP_SIZE_Z)]
void DebugDrawGrid(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid >= velocityGridResolution))
    {
        return;
    }
            
    float3 extents = float3(pressureGridExtents);
    float3 gridPos = float3(gridPosition); // origin is left-botom-back of the grid in world space
    const float3 gridCellSize = extents / pressureGridResolution;
    const float3 gridCellHalfSize = gridCellSize * 0.5f;
    const float3 cellPos = gridPos + (DTid * gridCellSize) + gridCellHalfSize; // centered pos
        
    // Adding Debugging lines to debug buffer
    if (all(DTid < pressureGridResolution))
    {
        bool3 isEdgeCell = bool3(DTid == (pressureGridResolution - (uint3) 1));
        DebugDrawCellEdges(cellPos, gridCellHalfSize, isEdgeCell);
        DebugDrawCellBoundaryCenter(cellPos, gridCellHalfSize);
    }
    
}

void Splat(RWTexture3D<float3> velocityGrid, uint3 baseCellIndex, float3 weight, float VelocityComponent)
{
    velocityGrid[baseCellIndex + uint3(0, 0, 0)].x = VelocityComponent * (1.f - weight.x) * (1.f - weight.y) * (1.f - weight.z);
    velocityGrid[baseCellIndex + uint3(1, 0, 0)].x = VelocityComponent * (weight.x) * (1.f - weight.y) * (1.f - weight.z);
    velocityGrid[baseCellIndex + uint3(0, 1, 0)].x = VelocityComponent * (1.f - weight.x) * (weight.y) * (1.f - weight.z);
    velocityGrid[baseCellIndex + uint3(1, 1, 0)].x = VelocityComponent * (weight.x) * (weight.y) * (1.f - weight.z);
    velocityGrid[baseCellIndex + uint3(0, 0, 1)].x = VelocityComponent * (1.f - weight.x) * (1.f - weight.y) * (weight.z);
    velocityGrid[baseCellIndex + uint3(1, 0, 1)].x = VelocityComponent * (weight.x) * (1.f - weight.y) * (weight.z);
    velocityGrid[baseCellIndex + uint3(0, 1, 1)].x = VelocityComponent * (1.f - weight.x) * (weight.y) * (weight.z);
    velocityGrid[baseCellIndex + uint3(1, 1, 1)].x = VelocityComponent * (weight.x) * (weight.y) * (weight.z);
}


[numthreads(PARTICLES_THREAD_GROUP_SIZE_X, PARTICLES_THREAD_GROUP_SIZE_Y, PARTICLES_THREAD_GROUP_SIZE_Z)]
void SplatParticlesToGrid(uint3 DTid : SV_DispatchThreadID) // Splat particle velocities to the grid.
{
    if (DTid.x >= particleCount)
    {
        return;
    }
    
    //Texture3D<float> pressureGridIn = ResourceDescriptorHeap[indexPressureGridInputIndex];
    RWTexture3D<float> pressureGridOut = ResourceDescriptorHeap[indexPressureGridOutputIndex];
    RWTexture3D<float3> velocityGridOut = ResourceDescriptorHeap[indexVelocityGridOutputIndex];
    
    
    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[indexParticleInputBuffer];
    //RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[indexParticleOutputBuffer];
    
    
    float3 extents = float3(pressureGridExtents);
    float3 gridPos = float3(gridPosition); // origin is left-botom-back of the grid in world space
    const float3 gridCellSize = extents / pressureGridResolution;
    //const float3 gridCellHalfSize = gridCellSize * 0.5f;
    //const float3 cellPos = gridPos + (DTid * gridCellSize) + gridCellHalfSize; // centered pos
    
    float3 fakeParticlePos = gridPos + (gridCellSize * float3(0.6f,0.6f,0.6f));
    ParticleGridData pgd = CalculateParticleGridData( fakeParticlePos, gridPos, gridCellSize);
    //ParticleGridData particleGridData = CalculateParticleGridData(particlesBufferIn[DTid.x].Pos, gridPos, gridCellSize);
    
    // There's 1 more cell in each direction in the velocity grid than the pressure grid.
    // we want to write the component wise velocity paramters of the particles to the 8 nearest velocity grid cells, 
    // weighted by baricentric weights. 
    
    //We will then normalize by the total weight contributed to each cell in a separate pass. - will we?
    float3 particleVel = particlesBufferIn[DTid.x].Vel;
    // Splat Horizontal
    Splat(velocityGridOut, pgd.baseCellIndexH, pgd.weightH, particleVel.x);
    //Splat(velocityGridOut, pgd.baseCellIndexV, pgd.weightV, particleVel.y);
    //Splat(velocityGridOut, pgd.baseCellIndexD, pgd.weightD, particleVel.z);
    
    //float3 velX = particleVel.x;
    //velocityGridOut[pgd.baseCellIndexH + uint3(0, 0, 0)].x = velX * (1.f - pgd.weightH.x) * (1.f - pgd.weightH.y) * (1.f - pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(1, 0, 0)].x = velX * (pgd.weightH.x) * (1.f - pgd.weightH.y) * (1.f - pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(0, 1, 0)].x = velX * (1.f - pgd.weightH.x) * (pgd.weightH.y) * (1.f - pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(1, 1, 0)].x = velX * (pgd.weightH.x) * (pgd.weightH.y) * (1.f - pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(0, 0, 1)].x = velX * (1.f - pgd.weightH.x) * (1.f - pgd.weightH.y) * (pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(1, 0, 1)].x = velX * (pgd.weightH.x) * (1.f - pgd.weightH.y) * (pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(0, 1, 1)].x = velX * (1.f - pgd.weightH.x) * (pgd.weightH.y) * (pgd.weightH.z);
    //velocityGridOut[pgd.baseCellIndexH + uint3(1, 1, 1)].x = velX * (pgd.weightH.x) * (pgd.weightH.y) * (pgd.weightH.z);
    
    
    // Test
    float FakePressure = 1.f;
    
    pressureGridOut[pgd.baseCellIndexH] = FakePressure;// * pgd.baricentricWeights.x * pgd.baricentricWeights.y * pgd.baricentricWeights.z;
    
    // There's going to be overwrites this way - we need to use atomic operations probably... although that will be slow...
    
}

[numthreads(PRESSUREGRID_THREAD_GROUP_SIZE_X, PRESSUREGRID_THREAD_GROUP_SIZE_Y, PRESSUREGRID_THREAD_GROUP_SIZE_Z)]
void ComputeGridDivergence(uint3 DTid : SV_DispatchThreadID)
{
    //if (any(DTid >= velocityGridResolution))
    //{
    //    return;
    //}
}


[numthreads(PRESSUREGRID_THREAD_GROUP_SIZE_X, PRESSUREGRID_THREAD_GROUP_SIZE_Y, PRESSUREGRID_THREAD_GROUP_SIZE_Z)]
void SolvePressureOnGrid(uint3 DTid : SV_DispatchThreadID)
{
    //if (any(DTid >= velocityGridResolution))
    //{
    //    return;
    //}
}

[numthreads(PARTICLES_THREAD_GROUP_SIZE_X, PARTICLES_THREAD_GROUP_SIZE_Y, PARTICLES_THREAD_GROUP_SIZE_Z)]
void InterpolateGridVelocityToParticles(uint3 DTid : SV_DispatchThreadID)
{
    //if (any(DTid >= velocityGridResolution))
    //{
    //    return;
    //}
}


[numthreads(PARTICLES_THREAD_GROUP_SIZE_X, PARTICLES_THREAD_GROUP_SIZE_Y, PARTICLES_THREAD_GROUP_SIZE_Z)]
void UpdateParticles(uint3 DTid : SV_DispatchThreadID)
{
    //if (any(DTid >= velocityGridResolution))
    //{
    //    return;
    //}
    
}

//[numthreads(PARTICLES_THREAD_GROUP_SIZE_X, PARTICLES_THREAD_GROUP_SIZE_Y, PARTICLES_THREAD_GROUP_SIZE_Z)]
//void CSMain(uint3 DTid : SV_DispatchThreadID)
//{
//    if (any(DTid >= velocityGridResolution))
//    {
//        return;
//    }
    
//    Texture3D<float> pressureGridIn = ResourceDescriptorHeap[indexPressureGridInputIndex];
//    RWTexture3D<float> pressureGridOut = ResourceDescriptorHeap[indexPressureGridOutputIndex];
    
//    StructuredBuffer<ParticleData> particlesBufferIn = ResourceDescriptorHeap[indexParticleInputBuffer];
//    RWStructuredBuffer<ParticleData> particlesBufferOut = ResourceDescriptorHeap[indexParticleOutputBuffer];
    
//    float3 extents = float3(pressureGridExtents);
//    int3 presGridRes = float3(pressureGridResolution);
//    float3 gridPos = float3(gridPosition); // origin is left-botom-back of the grid in world space
//    const float3 gridCellSize = extents / presGridRes;
//    const float3 gridCellHalfSize = gridCellSize * 0.5f;
//    const float3 cellPos = gridPos + (DTid * gridCellSize) + gridCellHalfSize; // centered pos
    
//    uint3 pressureGridCellIndex = min(DTid, pressureGridResolution - uint3(1,1,1)); // CLAMP pressure index as there's 1 fewer than velocity grid
    
//    float pressure = pressureGridIn[pressureGridCellIndex];
    
//    float3 accumulatedVel = float3(0.f, 0.f, 0.f);
//    int particleIndicesInCell[MAX_PARTICLES_PER_CELL];
//    int particleCountInCell = 0;
//    int i = 0;
//    for (; i < particleCount; ++i)
//    {
//        float3 particlePos = particlesBufferIn[i].Pos;
//        if (abs(particlePos.x - cellPos.x) < gridCellHalfSize.x &&
//            abs(particlePos.y - cellPos.y) < gridCellHalfSize.y &&
//            abs(particlePos.z - cellPos.z) < gridCellHalfSize.z)
//        {
//            accumulatedVel += particlesBufferIn[i].Vel;
//            particleIndicesInCell[particleCountInCell++] = i;
//            if (particleCountInCell == MAX_PARTICLES_PER_CELL)
//            {
//                break;
//            }
//        }
//    }
    
//    if (particleCountInCell > 0)
//    {
//        accumulatedVel /= particleCountInCell;
//        pressure = particleCountInCell;
//    }
//    else
//    {
//        pressure = 0.f;
//    }
//    pressureGridOut[pressureGridCellIndex] = pressure;
    
    
//    // Debugging
//    if (all(DTid < pressureGridResolution))
//    {
//        bool3 isEdgeCell = bool3(DTid == (pressureGridResolution - (uint3)1));
//        DebugDrawCellEdges(cellPos, gridCellHalfSize, isEdgeCell, pressure);
//        DebugDrawCellBoundaryCenter(cellPos, gridCellHalfSize);
//    }
    
//    // Update each particle within the bounds of this lane's cell
//    for (uint particleInCellIdx = 0; particleInCellIdx < particleCountInCell; ++particleInCellIdx)
//    {
//        i = particleIndicesInCell[particleInCellIdx];
        
//        float3 particlePos = particlesBufferIn[i].Pos;
//        if (abs(particlePos.x - cellPos.x) < gridCellHalfSize.x &&
//            abs(particlePos.y - cellPos.y) < gridCellHalfSize.y &&
//            abs(particlePos.z - cellPos.z) < gridCellHalfSize.z)
//        {
//            // Cell velocity
//            particlesBufferOut[i].Vel += accumulatedVel * (1.f / 60.f);
//            // Gravity
//            particlesBufferOut[i].Vel += float3(0.f, -98.1f, 0.f) * (1.f / 60.f);
            
            
//            // Update position
//            particlesBufferOut[i].Pos = particlePos + particlesBufferOut[i].Vel * (1.f / 60.f);
            
//            // Clamp to bounds
//            const float radius = 1.0f;
//            bool applyFriction = false;
//            if (particlesBufferOut[i].Pos.x - radius < gridPos.x)
//            {
//                particlesBufferOut[i].Pos.x = gridPos.x + radius;
//                particlesBufferOut[i].Vel.x *= -0.5f;
//                applyFriction = true;
//            }
//            else if (particlesBufferOut[i].Pos.x + radius > gridPos.x + pressureGridExtents.x)
//            {
//                particlesBufferOut[i].Pos.x = gridPos.x + pressureGridExtents.x - radius;
//                particlesBufferOut[i].Vel.x *= -0.5f;
//                applyFriction = true;
//            }
            
//            if (particlesBufferOut[i].Pos.y - radius < gridPos.y)
//            {
//                particlesBufferOut[i].Pos.y = gridPos.y + radius;
//                particlesBufferOut[i].Vel.y *= -0.5f;
//                applyFriction = true;
//            }
//            else if (particlesBufferOut[i].Pos.y - radius > gridPos.y + pressureGridExtents.y)
//            {
//                particlesBufferOut[i].Pos.y = gridPos.y + pressureGridExtents.y - radius;
//                particlesBufferOut[i].Vel.y *= -0.5f;
//                applyFriction = true;
//            }
            
//            if (particlesBufferOut[i].Pos.z - radius < gridPos.z)
//            {
//                particlesBufferOut[i].Pos.z = gridPos.z + radius;
//                particlesBufferOut[i].Vel.z *= -0.5f;
//                applyFriction = true;
//            }
//            else if (particlesBufferOut[i].Pos.z + radius > gridPos.z + pressureGridExtents.z)
//            {
//                particlesBufferOut[i].Pos.z = gridPos.z + pressureGridExtents.z - radius;
//                particlesBufferOut[i].Vel.z *= -0.5f;
//                applyFriction = true;
//            }
            
//            if(applyFriction)
//            {
//                particlesBufferOut[i].Vel *= 0.98f;
//            }
//        }
//    }
//}