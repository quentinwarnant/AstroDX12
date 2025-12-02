#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float2> VelocityTex : register(t0);
RWTexture2D<float> Divergence : register(u0);

cbuffer BindlessRenderResources : register(b0)
{
    uint GridResolution;
}


uint2 SafeCoord(uint2 coord)
{
    return uint2(clamp(coord.x, 0, GridResolution - 1), clamp(coord.y, 0, GridResolution - 1));
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float dx = 1.0f / GridResolution;
    
    float2 Vel_Left = VelocityTex[SafeCoord(DTid.xy + uint2(-1, 0))];
    float2 Vel_Right = VelocityTex[SafeCoord(DTid.xy + uint2(1, 0))];
    float2 Vel_Down = VelocityTex[SafeCoord(DTid.xy + uint2(0, 1))]; // origin is top left
    float2 Vel_Up = VelocityTex[SafeCoord(DTid.xy + uint2(0, -1))];
    
    float divergence = 0.5f * (Vel_Right.x - Vel_Left.x + Vel_Up.y - Vel_Down.y) * dx;
    
    Divergence[DTid.xy] = divergence;
}