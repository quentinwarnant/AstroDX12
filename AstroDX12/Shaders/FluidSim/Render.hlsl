
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
    int gridDimension;
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
    float2 sampledCol = simTexture.Sample(g_BindlessSamplers[samplerIndex], i.uv);
    
    sampledCol = sampledCol * 0.5f + 0.5f; // map from [-1,1] to [0,1]
    //sampledCol *= 1.f; // scale for better visibility
    
    
    float borderCol = 0.f;
    
    const float texSize = gridDimension;
    if (i.uv.x < 1.f / texSize || i.uv.x > (texSize - 1.f) / texSize ||
       i.uv.y < 1.f / texSize || i.uv.y > (texSize-1.f) / texSize)
    {
        borderCol = 1.f;

    }
    return float4(sampledCol.r, sampledCol.g, borderCol, 1);
}
