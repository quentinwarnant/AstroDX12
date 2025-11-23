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
    const float deltaTime = 1.f / 60.f;
    
    // backwards advection
    float2 velocityCurrentCell = VelocityGridInput[DTid.xy];
    uint2 backTracedCellOffset = velocityCurrentCell * GridResolution * deltaTime;
    uint2 backTracedCellID = DTid.xy - backTracedCellOffset;
    //TODO: check we're sampling the right cell here with rounding down being inferred that shouldn't be
    
    
    VelocityGridOutput[DTid.xy] = VelocityGridInput[backTracedCellID];
}