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
    int chainElementBufferIndex;
    int modelVertexDataBufferIdx;
};

struct ParticleData
{
    float3 Pos;
};


struct ChainElementData
{
    ParticleData Particle;
    int ParentIndex;
    float RestLength;
    bool Pinned;
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

    StructuredBuffer<ChainElementData> chainElementData = ResourceDescriptorHeap[chainElementBufferIndex];
    StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[modelVertexDataBufferIdx];

	// Transform to homogeneous clip space.
    const float3 posL = vertexData[VertexID].posLocal;
    const float3 posW = posL + chainElementData[InstanceID].Particle.Pos;
    o.PosH = mul(float4(posW, 1.0f), gViewProj);
    o.Color = chainElementData[InstanceID].Pinned ? float4(1.f, 0.f, 0.f, 1.f) : float4(1.f, 1.f, 1.f, 1.f);

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return i.Color;
}
