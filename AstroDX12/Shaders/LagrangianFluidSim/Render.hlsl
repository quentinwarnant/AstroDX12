
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
    int particlesTransformBufferIndex;
    int modelVertexDataBufferIdx;
};

struct VertexData
{
    float3 posLocal;
};

struct ParticleData
{
    float3 position;
    float3 velocity;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 normalWorld : NORMAL;
};

PSInput VS(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    PSInput o;

    StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[modelVertexDataBufferIdx];
    StructuredBuffer<ParticleData> particlesTransformBuffer = ResourceDescriptorHeap[particlesTransformBufferIndex];
    float3 particleWorldPos = particlesTransformBuffer[InstanceID].position;
    
	// Transform to homogeneous clip space.
    const float3 posL = vertexData[VertexID].posLocal;
    const float3 posW = posL + particleWorldPos;
    o.posH = mul(float4(posW, 1.0f), gViewProj);
    
    float3 normal = normalize(posL);
    o.normalWorld = mul(float4(normal, 0.0f), gViewProj).xyz;
    
    
    return o;
}

float4 PS(PSInput i) : SV_Target
{
    float3 lightDir = normalize(float3(-0.f, 1.f, -1.577f));
    float nDotL = dot(normalize(i.normalWorld), lightDir);
    
    float3 color = saturate(nDotL) * float3(0.2f, 0.5f, 1.0f) + float3(0.1f, 0.1f, 0.1f);
    
    return float4(color, 1);
}
