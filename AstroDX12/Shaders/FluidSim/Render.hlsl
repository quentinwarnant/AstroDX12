
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

SamplerState g_BindlessSamplers[] : register(s0);

cbuffer BindlessRenderResources : register(b1)
{
    int imageIndex;
    int samplerIndex;
    int modelVertexDataBufferIdx;
};

struct VertexData
{
    float3 posLocal;
    float2 uv;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput VS(uint VertexID : SV_VertexID)
{
    PSInput o;

    StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[modelVertexDataBufferIdx];

	// Transform to homogeneous clip space.
    const float3 posL = vertexData[VertexID].posLocal;
    const float3 posW = posL; // TODO: transform by world matrix 
    o.posH = mul(float4(posW, 1.0f), gViewProj);
    o.uv = vertexData[VertexID].uv;
    
    return o;
}

float4 PS(PSInput i) : SV_Target
{
    Texture2D<float2> simTexture = ResourceDescriptorHeap[imageIndex];
    float2 color = simTexture.Sample(g_BindlessSamplers[samplerIndex], i.uv);
    return float4(color.r, color.g, 0, 1);
}
