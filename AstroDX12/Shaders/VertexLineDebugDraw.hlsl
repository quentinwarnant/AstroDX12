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

float4 GetDebugColor(int index)
{
    const float4 DebugColors[9] = {
        float4(1, 0, 0, 1), // Red
        float4(0, 1, 0, 1), // Green
        float4(0, 0, 1, 1), // Blue
        float4(1, 1, 0, 1), // Yellow
        float4(1, 0, 1, 1), // Magenta
        float4(0, 1, 1, 1), // Cyan
        float4(1, 0.5f, 0, 1), // Orange
        float4(0.5f, 0, 1, 1), // Purple
        float4(1.0f, 1.0f, 1.0f, 1) // white 
    };
    
    return DebugColors[index];
}

PSInput VS(uint VertexID : SV_VertexID, uint InstanceID : SV_InstanceID)
{
    PSInput o;

    ConstantBuffer<GlobalSceneData> SceneData = ResourceDescriptorHeap[globalCBVIdx];
    StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[debugDrawLineVertexDataBufferIdx];
    
	// Transform to homogeneous clip space.
    const float3 posW = vertexData[VertexID].posWorld;
    o.PosH = mul(float4(posW, 1.0f), SceneData.gViewProj); 
    o.Color = GetDebugColor(vertexData[VertexID].colorIndex);

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    return i.Color;
}
