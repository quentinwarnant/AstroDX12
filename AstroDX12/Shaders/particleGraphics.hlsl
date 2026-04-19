#include "Shaders/ParticlesCommon.hlsli"

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
    int particlesBufferIndex;
    int modelVertexDataBufferIdx;
};

struct VertexData
{
    float3 posLocal;
};

struct PSInput
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

PSInput VS(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    PSInput o;

    StructuredBuffer<ParticleData> particleData = ResourceDescriptorHeap[particlesBufferIndex];
    StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[modelVertexDataBufferIdx];

	// Transform to homogeneous clip space.
    const float3 posL = vertexData[VertexID].posLocal * particleData[InstanceID].Size;
    const float3 posW = posL + particleData[InstanceID].Pos;
    o.PosH = mul(float4(posW, 1.0f), gViewProj);
    
    float3 normalWS = posL;
    static const float3 fakeLightDir = normalize(float3(0.5f, 1.0f, -0.5f));
    static const float3 fakeLightColor = normalize(float3(1.5f, 1.0f, 0.0f));
    const float nDotL = saturate(dot(normalWS, fakeLightDir));
    const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
    o.Color = float4(ambientColor + (nDotL * fakeLightColor), 1.f);

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return i.Color;
}
