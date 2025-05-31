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
    int objectConstantsBufferIndex;
    int objectIdx;
}

struct ObjectConstants
{
    float4x4 gWorld;
};

struct VertexData
{
    float3 posLocal;
    float3 normal;
    float2 uv;
};

struct PSInput
{
    float4 PosH : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;

};

PSInput VS(uint VertexID : SV_VertexID)
{
    PSInput o;

    StructuredBuffer<ObjectConstants> objectConstantsBuffer = ResourceDescriptorHeap[objectConstantsBufferIndex];
    StructuredBuffer<VertexData> positionBuffer = ResourceDescriptorHeap[positionBufferIndex];

	// Transform to homogeneous clip space.
    const float3 posL = positionBuffer[VertexID].posLocal;
    const float4 posW = mul(float4(posL, 1.0f), objectConstantsBuffer[objectIdx].gWorld);
    o.PosH = mul(posW, gViewProj);
    o.Normal = mul(float4(positionBuffer[VertexID].normal, 0.f), objectConstantsBuffer[objectIdx].gWorld).xyz;
    o.UV = positionBuffer[VertexID].uv;

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    const float3 fakeLightDir = normalize(float3(-0.2f, -0.8f, 0.3f));
    const float3 normal = normalize(i.Normal);
    const float nDotL = max(0.1, dot(normal, fakeLightDir));
    const float4 color = float4(0.8f, 0.1f, 0.2f, 1.f) * nDotL;
    
    return color;
}
