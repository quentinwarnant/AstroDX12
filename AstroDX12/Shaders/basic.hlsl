//TODO: move to include file - begin
cbuffer cbPerObject : register(b2)
{
    float4x4 gWorld;
};

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
    int positionBufferIndex;
}

// ---- TODO move to include file - end

struct VSInput
{
    float3 PosL : POSITION;
};

struct PSInput
{
    float4 PosH : SV_POSITION;
};

struct VertexData
{
    float3 posLocal;
};

PSInput VS(uint VertexID : SV_VertexID)
{
    PSInput o;

	// Transform to homogeneous clip space.
    StructuredBuffer<VertexData> positionBuffer = ResourceDescriptorHeap[positionBufferIndex];

	// Transform to homogeneous clip space.
    const float3 posL = positionBuffer[VertexID].posLocal;
    const float4 posW = mul(float4(posL, 1.0f), gWorld);
    o.PosH = mul(posW, gViewProj);

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return float4(0.8f, 0.8f, 0.8f, 1.f);
}