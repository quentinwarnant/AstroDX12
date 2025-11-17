#include "Shaders/physicsChainCommon.hlsli"
#include "Shaders/Inc/DebugDrawData.hlsli"


struct CollisionElement
{
    float3 Pos;
    float Radius;
};

#define MAX_COLLISION_ELEMENTS 8
struct CollisionCollectionData
{
    CollisionElement Elements[MAX_COLLISION_ELEMENTS];
    int ElementCount;
};

cbuffer BindlessRenderResources : register(b0)
{
    int BindlessIndexChainElementBufferInput;
    int BindlessIndexChainElementBufferOutput;
    int SimNeedsReset;
    int DebugDrawBufferUAVIndex;
}

#define GROUP_SIZE 32

groupshared float3 Pos[GROUP_SIZE];
groupshared float3 PrevPos[GROUP_SIZE];
groupshared bool Pinned[GROUP_SIZE];
groupshared float RestLengths[GROUP_SIZE];
groupshared int ParentIndex[GROUP_SIZE];
groupshared float NodeRadii[GROUP_SIZE];

CollisionCollectionData MakeCollisionData()
{
    CollisionCollectionData data;
    data.Elements[0].Pos = float3(16.f, -24.f, 9.f);
    data.Elements[0].Radius = 5.f;
    
    data.Elements[1].Pos = float3(18.f, -35.f, 10.f);
    data.Elements[1].Radius = 3.f;
    
    data.Elements[2].Pos = float3(10.f, -47.f, 10.f);
    data.Elements[2].Radius = 3.5f;

    data.ElementCount = 3;
    return data;
}

void HandleConstraints(inout float3 nodePos, inout float3 parentNodePos, in float restLength, in bool isParentPinned)
{
    // Only length constraint for now
    const float3 NodeToParent = parentNodePos - nodePos;
    const float distanceToParent = length(NodeToParent);
    const float lengthCorrection = distanceToParent - restLength;
    if (abs(lengthCorrection) > 0.0001f)
    {
        const float3 correctionDir = normalize(NodeToParent) * lengthCorrection;
        float w1 = isParentPinned ? 1.f : 0.5f;
        float w2 = 1.f - w1;
        nodePos += correctionDir * w1;
        parentNodePos -= correctionDir * w2;
    }
}

void HandleCollisions(inout float3 nodePos, in float nodeRadius, in CollisionCollectionData collisionData)
{
    for (int i = 0; i < collisionData.ElementCount; ++i)
    {
        const CollisionElement element = collisionData.Elements[i];
        const float3 toElement = element.Pos - nodePos;
        const float distance = length(toElement);
        if (distance < element.Radius + nodeRadius)
        {
            // Simple collision response - push the node out of the element
            const float3 pushDir = normalize(toElement);
            float diff = (element.Radius + nodeRadius) - distance;
            nodePos += -pushDir * diff;
        }
    }
}

void PBDSolver(float dt, float3 externalForces, uint elementCount, in CollisionCollectionData collisionData)
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
                
                HandleCollisions(newPos, NodeRadii[i], collisionData);
                HandleConstraints(newPos, Pos[parentIdx], RestLengths[i], Pinned[parentIdx]);
                
            
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
    RWStructuredBuffer<DebugObjectData> drawDebugBufferOut = ResourceDescriptorHeap[DebugDrawBufferUAVIndex];
    
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
    NodeRadii[DTid.x] = Data.Radius;
    
    GroupMemoryBarrierWithGroupSync();
        // Tmp - create collision data in shader for now
    const CollisionCollectionData collisionData = MakeCollisionData();

    if (DTid.x == 0)
    {
        PBDSolver(dt, gravity, elementCount, collisionData);
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
    
    //--------------------------------------
    // Debug visualsiation
    // Visualise the Chain Nodes
    const float size = Data.Radius * 3.f;
    drawDebugBufferOut[DTid.x].Transform = float4x4(
        float4(size, 0.f, 0.f, 0.f),
        float4(0.f, size, 0.f, 0.f),
        float4(0.f, 0.f, size, 0.f),
        float4(Data.Particle.Pos.x, Data.Particle.Pos.y, Data.Particle.Pos.z, 1.f)
    );
    drawDebugBufferOut[DTid.x].Color = float3(0.f, 0.f, 1.f);
    
    //Visualise the collision elements
    if (DTid.x == 0)
    {
        for(int i = 0; i < collisionData.ElementCount; ++i)
        {
            const float3 pos = collisionData.Elements[i].Pos;
            const float radius = collisionData.Elements[i].Radius;
            
            drawDebugBufferOut[LANECOUNT + i].Transform = float4x4(
                float4(radius * 2.f, 0.f, 0.f, 0.f),
                float4(0.f, radius * 2.f, 0.f, 0.f),
                float4(0.f, 0.f, radius * 2.f, 0.f),
                float4(pos.x, pos.y, pos.z, 1.f)
            );
            drawDebugBufferOut[LANECOUNT + i].Color = float3(1.f, 1.f, 0.f);

        }
    }
    //--------------------------------------
}