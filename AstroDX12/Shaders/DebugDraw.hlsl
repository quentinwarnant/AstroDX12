#include "Shaders/Inc/DebugDrawData.hlsli"

cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 InvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float gPerObjectPad1; // Unused - padding
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

cbuffer BindlessRenderResources : register(b1)
{
    int vertexDataBufferIndex;
    int debugObjectDataBufferIndex;
}


struct PSInput
{
    float4 PosH : SV_POSITION;
    float3 Color: COLOR0;
};

struct VertexData
{
    float3 posLocal;
};

PSInput VS(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    PSInput o;

    StructuredBuffer<DebugObjectData> objectConstantsBuffer = ResourceDescriptorHeap[debugObjectDataBufferIndex];
    StructuredBuffer<VertexData> vertexBuffer = ResourceDescriptorHeap[vertexDataBufferIndex];
	
	// Transform to homogeneous clip space.
    const float3 posL = vertexBuffer[VertexID].posLocal;
    const float4 posW = mul(float4(posL, 1.0f), objectConstantsBuffer[InstanceID].Transform);
    o.PosH = mul(posW, gViewProj);
    o.Color = objectConstantsBuffer[InstanceID].Color;

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return float4(i.Color, 1.f);
}
