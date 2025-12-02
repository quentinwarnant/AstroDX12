#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float2> velocityTexIn : register(t0);
Texture2D<float> pressureTex : register(t1);
RWTexture2D<float2> velocityTexOut : register(u0);

cbuffer BindlessRenderResources : register(b0)
{
    int GridResolution;
}

int2 SafeCoord(int2 coord)
{
    return int2(clamp(coord.x, 0, GridResolution - 1), clamp(coord.y, 0, GridResolution - 1));
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    const float cellSize = 1.f / GridResolution;
    const float2 currentVelocity = velocityTexIn[DTid.xy];
    
    int2 coord = int2(DTid.xy);
    
    const float p_up = pressureTex[SafeCoord(coord + int2(0, -1))];
    const float p_down = pressureTex[SafeCoord(coord + int2(0, 1))];
    const float p_right = pressureTex[SafeCoord(coord + int2(1, 0))];
    const float p_left = pressureTex[SafeCoord(coord + int2(-1, 0))];
    
    const float cellSpacing = 1.f / (2.f * cellSize);
    const float2 pressureGradient = float2(p_right - p_left, p_up - p_down) * 0.5 * cellSpacing;
    
    const float2 newVelocity = currentVelocity - pressureGradient;
    
    velocityTexOut[DTid.xy] = newVelocity;
}

//Done after projection
[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain_ReflectEdgeVelocity(uint3 DTid : SV_DispatchThreadID)
{
    // reflect velocity at the edges
    float2 velocity = velocityTexOut[DTid.xy];
    if (DTid.x == 0)
    {
        velocity = float2(-1.f, 1.f) * velocityTexOut[uint2(1, DTid.y)];
    }
    else if (DTid.x == GridResolution - 1)
    {
        velocity = float2(-1.f, 1.f) * velocityTexOut[uint2(GridResolution - 2, DTid.y)];
    }

    if (DTid.y == 0)
    {
        velocity = float2(1.f, -1.f) * velocityTexOut[uint2(DTid.x, 1)];
    }
    else if (DTid.y == GridResolution - 1)
    {
        velocity = float2(1.f, -1.f) * velocityTexOut[uint2(DTid.x, GridResolution - 2)];
    }

    velocityTexOut[DTid.xy] = velocity;
}