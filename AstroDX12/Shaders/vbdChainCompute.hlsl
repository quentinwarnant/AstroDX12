#include "Shaders/vbdChainCommon.hlsli"
#include "Shaders/Inc/DebugDrawData.hlsli"

cbuffer BindlessRenderResources : register(b0)
{
    int BindlessIndexChainElementBufferInput;
    int BindlessIndexChainElementBufferOutput;
    int SimNeedsReset;
    int DebugDrawBufferUAVIndex;
    int DebugDrawCounterUAVIndex;
};

#define GROUP_SIZE 32

groupshared float3 Pos[GROUP_SIZE];
groupshared float3 PrevPos[GROUP_SIZE];
groupshared float3 InertialPos[GROUP_SIZE]; // x̃ — the inertial prediction
groupshared bool Pinned[GROUP_SIZE];
groupshared float RestLengths[GROUP_SIZE];
groupshared int ParentIndex[GROUP_SIZE];
groupshared float NodeRadii[GROUP_SIZE];


//=============================================================================
// TODO: You will implement this function step by step!
//
// VBD Solver — the core algorithm:
//   1. Compute inertial prediction x̃ for each link
//   2. For each Gauss-Seidel iteration:
//      a. For each non-pinned link:
//         - Assemble gradient g and Hessian H from inertia + constraints
//         - Solve 3x3 system: H * dx = -g
//         - Update position: x -= dx
//         - Handle collisions
//=============================================================================
void VBDSolver(float dt, float3 externalForces, uint elementCount)
{
    // Your implementation goes here!
}


float3x3 CalculateRotationMatrix(float3 direction)
{
    const float epsilon = 0.001f;
    if (abs(direction.x) < epsilon && abs(direction.y) < epsilon && abs(direction.z) < epsilon) 
    {
        direction = float3(0.f, 1.f, 0.f);
    }
    
    float3 yAxis = normalize(direction);
    float3 xAxis = normalize(cross(float3(0.f, 0.f, 1.f), yAxis));
    float3 zAxis = cross(xAxis, yAxis);
    
    return float3x3(xAxis, yAxis, zAxis);
}

void ResetChain(inout VBDChainElementData data, int idx)
{
    const float3 origin = float3(10.f, 0.f, 10.f);
    data.Particle.Pos = origin + float3(idx * data.RestLength, 0.f, 0.f);
    data.Particle.PrevPos = data.Particle.Pos;
}

#define LANECOUNT 15
[numthreads(LANECOUNT, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    StructuredBuffer<VBDChainElementData> chainDataBufferIn = ResourceDescriptorHeap[BindlessIndexChainElementBufferInput];
    RWStructuredBuffer<VBDChainElementData> chainDataBufferOut = ResourceDescriptorHeap[BindlessIndexChainElementBufferOutput];
    RWStructuredBuffer<DebugObjectData> drawDebugBufferOut = ResourceDescriptorHeap[DebugDrawBufferUAVIndex];
    
    VBDChainElementData Data = chainDataBufferIn[DTid.x];
    
    if (SimNeedsReset == 1)
    {
        ResetChain(Data, DTid.x);
    }
    
    const float dt = 1.f / 60.f;
    const float3 gravity = float3(0.f, -9.81f, 0.f);
    const uint elementCount = LANECOUNT;
    
    // Load into groupshared memory
    Pos[DTid.x] = Data.Particle.Pos;
    PrevPos[DTid.x] = Data.Particle.PrevPos;
    Pinned[DTid.x] = Data.Pinned;
    RestLengths[DTid.x] = Data.RestLength;
    ParentIndex[DTid.x] = Data.ParentIndex;
    NodeRadii[DTid.x] = Data.Radius;
    
    GroupMemoryBarrierWithGroupSync();

    // Thread 0 runs the VBD solver sequentially (Gauss-Seidel)
    if (DTid.x == 0)
    {
        VBDSolver(dt, gravity, elementCount);
    }
    GroupMemoryBarrierWithGroupSync();
    
    // Write results back
    Data.Particle.PrevPos = Data.Particle.Pos;
    Data.Particle.Pos = Pos[DTid.x];
    
    uint RotationRefNodeTopIndex = DTid.x;
    uint RotationRefNodeBottomIndex = DTid.x+1;
    if (DTid.x == elementCount - 1)
    {
        RotationRefNodeTopIndex = DTid.x-1;
        RotationRefNodeBottomIndex = DTid.x;
    }
    Data.Particle.Rot = CalculateRotationMatrix(Pos[RotationRefNodeTopIndex] - Pos[RotationRefNodeBottomIndex]);

    chainDataBufferOut[DTid.x] = Data;
    
    //--------------------------------------
    // Debug visualisation - allocate slots via atomic counter
    RWStructuredBuffer<uint> drawDebugCounterOut = ResourceDescriptorHeap[DebugDrawCounterUAVIndex];
    
    uint mySlot;
    InterlockedAdd(drawDebugCounterOut[0], 1, mySlot);
    
    const float size = Data.Radius * 3.f;
    drawDebugBufferOut[mySlot].Transform = float4x4(
        float4(size, 0.f, 0.f, 0.f),
        float4(0.f, size, 0.f, 0.f),
        float4(0.f, 0.f, size, 0.f),
        float4(Data.Particle.Pos.x, Data.Particle.Pos.y, Data.Particle.Pos.z, 1.f)
    );
    drawDebugBufferOut[mySlot].Color = float3(0.5f, 0.f, 1.f); // Purple to distinguish from PBD (blue) and AVBD (green)
    //--------------------------------------
}
