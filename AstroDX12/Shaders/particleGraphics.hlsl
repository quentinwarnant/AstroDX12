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

struct ParticleData
{
    float3 Pos;
    float3 Vel;
    float Age;
    float Lifetime;
};

struct VertexData
{
    float3 posLocal;
    float4 color;
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
    const float3 posL = vertexData[VertexID].posLocal;
    const float3 posW = posL + particleData[InstanceID].Pos;
    o.PosH = mul(float4(posW, 1.0f), gViewProj);
    o.Color = vertexData[VertexID].color;

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return i.Color;
}
