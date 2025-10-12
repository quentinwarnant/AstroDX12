#include "Shaders/physicsChainCommon.hlsli"

cbuffer BindlessRenderResources : register(b0)
{
    int BindlessIndexChainElementBufferInput;
    int BindlessIndexChainElementBufferOutput;
    int SimNeedsReset;
}

#define GROUP_SIZE 32

groupshared float3 Pos[GROUP_SIZE];
groupshared float3 PrevPos[GROUP_SIZE];
groupshared bool Pinned[GROUP_SIZE];
groupshared float RestLengths[GROUP_SIZE];
groupshared int ParentIndex[GROUP_SIZE];

void PBDSolver(float dt, float3 externalForces, uint elementCount)
{
    const uint iterationCount = 30;
    const float dtIt = dt / iterationCount;
    const float dtItSqr = dtIt * dtIt;
    float3 velocity[GROUP_SIZE];
    for(uint i = 0; i < elementCount; ++i)
    {
        velocity[i] = (Pos[i] - PrevPos[i]) / dt;
    }
        
    for(uint iter = 0; iter < iterationCount; ++iter)
    {
        for(uint i = 0; i < elementCount; ++i)
        {
            if (!Pinned[i])
            {
                float3 newPos = Pos[i];
                const float dampeningFactor = 0.99f;
                newPos += velocity[i] * dtIt * dampeningFactor; // velocity + Damping
                // TODO: factor in acceleration 
                newPos += externalForces * dtIt;

                const uint parentIdx = ParentIndex[i];
                
                const float3 NodeToParent = Pos[parentIdx] - newPos;
                const float distanceToParent = length(NodeToParent);
                const float lengthCorrection = distanceToParent - RestLengths[i];
                if (abs(lengthCorrection) > 0.0001f)
                {
                    const float3 correctionDir = normalize(NodeToParent) * lengthCorrection;
                    const bool isParentPinned = Pinned[parentIdx];
                    float w1 = isParentPinned ? 1.f : 0.5f;
                    float w2 = 1.f - w1;
                    newPos += correctionDir * w1;
                    Pos[parentIdx] -= correctionDir * w2;
                }
            
                Pos[i] = newPos;
            }
        }
    }
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

void ResetChain(inout ChainElementData data, int idx)
{
    const float3 origin = float3(10.f, 0.f, 10.f);
    data.Particle.Pos = origin + float3(idx * data.RestLength, 0.f, 0.f);
    data.Particle.PrevPos = data.Particle.Pos;
}

#define LANECOUNT 15
[numthreads(LANECOUNT, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    StructuredBuffer<ChainElementData> chainDataBufferIn = ResourceDescriptorHeap[BindlessIndexChainElementBufferInput];
    RWStructuredBuffer<ChainElementData> chainDataBufferOut = ResourceDescriptorHeap[BindlessIndexChainElementBufferOutput];
    
    ChainElementData Data = chainDataBufferIn[DTid.x];
    
    if (SimNeedsReset == 1)
    {
        ResetChain(Data, DTid.x);
    }
    
    const float dt = 1.f / 60.f; // should be an input arg
    const float3 gravity = float3(0.f, -9.81f, 0.f);
    const uint elementCount = LANECOUNT; //TODO: should be an input arg
    
    Pos[DTid.x] = Data.Particle.Pos;
    PrevPos[DTid.x] = Data.Particle.PrevPos;
    Pinned[DTid.x] = Data.Pinned;
    RestLengths[DTid.x] = Data.RestLength;
    ParentIndex[DTid.x] = Data.ParentIndex;
    
    GroupMemoryBarrierWithGroupSync();
    if (DTid.x == 0)
    {
        PBDSolver(dt, gravity, elementCount);
    }
    GroupMemoryBarrierWithGroupSync();
    
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
}