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
    
    VelocityGridOutput[DTid.xy] = VelocityGridInput[DTid.xy] + (0.01f * step(0.1f, length(uv)) );
}