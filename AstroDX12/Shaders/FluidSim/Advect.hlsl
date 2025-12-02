#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float2> VelocityGridInput : register(t0);
RWTexture2D<float2> VelocityGridOutput : register(u0);

SamplerState g_BindlessSamplers[] : register(s0);

cbuffer BindlessRenderResources : register(b0)
{
    int GridResolution;
    int samplerIndex;
}

bool IsEdge(uint2 coord)
{
    float edgeWidth = 5.f;
    return (coord.x < edgeWidth || coord.x > GridResolution - edgeWidth || coord.y < edgeWidth || coord.y > GridResolution - edgeWidth);
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    const float deltaTime = 1.f / 60.f;
    
    // backwards advection
    float2 velocityCurrentCell = VelocityGridInput[DTid.xy];
    float2 backTracedCellOffset = (float2(1.f, -1.f) * velocityCurrentCell * deltaTime) / float(GridResolution);
    const float2 texelCenterOffset = (float2) 1.f / GridResolution * 0.5f;
    float2 currentUV = float2(DTid.xy) / float(GridResolution) + texelCenterOffset;
    float2 targetUV = saturate(currentUV - backTracedCellOffset);
                   
    float2 interpolatedVelocityValue = VelocityGridInput.Sample(g_BindlessSamplers[samplerIndex], targetUV);

    VelocityGridOutput[DTid.xy] = interpolatedVelocityValue;
}