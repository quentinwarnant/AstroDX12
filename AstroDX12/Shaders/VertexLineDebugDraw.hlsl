cbuffer BindlessRenderResources : register(b0)
{
    int debugDrawLineVertexDataBufferIdx;
	int globalCBVIdx;
};

struct GlobalSceneData
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

struct VertexData
{
    float3 posWorld;
    int colorIndex;
};

struct PSInput
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

PSInput VS(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    PSInput o;

    ConstantBuffer<GlobalSceneData> SceneData = ResourceDescriptorHeap[globalCBVIdx];
    StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[debugDrawLineVertexDataBufferIdx];

    
	// Transform to homogeneous clip space.
    const float3 posW = vertexData[VertexID].posWorld;
    o.PosH = mul(float4(posW, 1.0f), SceneData.gViewProj); // when we've plugged the CBV we can use this
    //o.PosH = float4(posW, 1.0f);
    o.Color = float4(0, 1, 0, 1); // TODO : use color index to pick color

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return i.Color;
}
