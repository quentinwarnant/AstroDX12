#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float2> VelocityGridInput : register(t0);
RWTexture2D<float2> VelocityGridOutput : register(u0);

cbuffer BindlessRenderResources : register(b0)
{
    int GridResolution;
}


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = (float2(DTid.xy) + 0.5f) / float2(GridResolution, GridResolution);
    uv -= 0.5f;
    uv *= 0.5f;
    
    const float2 addedVelocityDir = normalize(float2(1.f, 0.5f));
    const float addedVelocityStrength = 0.1f;
    VelocityGridOutput[DTid.xy] += addedVelocityDir * addedVelocityStrength * step(length(uv), 0.05f);
}