#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float2> VelocityGridInput : register(t0);
RWTexture2D<float2> VelocityGridOutput : register(u0);

cbuffer BindlessRenderResources : register(b0)
{
    int GridResolution;
    float time;
}

float2 GetDir(float t)
{
    float angle = t * 3.1415f * 0.2f;
    return float2(cos(angle), sin(angle));
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float2 uv = (float2(DTid.xy) + 0.5f) / float2(GridResolution, GridResolution);
    uv -= 0.5f;
    uv *= 0.5f;    
    uv.x -= 0.15f;
    
    const float deltaTime = 1.f / 60.f; // todo make this an input

    const float2 dir = GetDir(time);
    const float2 addedVelocityDir = normalize(dir);
    const float addedVelocityStrength = 10000.04f * deltaTime;
    
    float2 currentVelocity = VelocityGridInput[DTid.xy];
    float2 inputVelocity = addedVelocityDir * addedVelocityStrength;
    const float maskRadius = 0.02f;
    float mask = step(length(uv), maskRadius);
    VelocityGridOutput[DTid.xy] = lerp(currentVelocity, currentVelocity + (inputVelocity), mask);
}