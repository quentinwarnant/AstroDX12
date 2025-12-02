#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float> DivergenceTex : register(t0);
Texture2D<float> PressureTexIn : register(t1);
RWTexture2D<float> PressureTexOut : register(u0);

SamplerState g_BindlessSamplers[] : register(s0);


cbuffer BindlessRenderResources : register(b0)
{
    int GridResolution;
    uint samplerIndex;
}

int2 SafeCoord(int2 coord)
{
    return int2(clamp(coord, int2(0, 0), (int2) GridResolution - 1) );
}


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float h = 1.0f / GridResolution; // grid spacing
    float h2 = h * h; // grid spacing , squared
    float invGridSpacing = 1.f / h2;
    
    int2 coord = int2(DTid.xy);
    
    
    const float2 centerOfCellOffset = float2(0.5f, 0.5f) / GridResolution;
    const float2 uv = centerOfCellOffset + DTid.xy / float(GridResolution);
    const float div = DivergenceTex.Sample(g_BindlessSamplers[samplerIndex], uv);
    
    const float2 uvLeft = centerOfCellOffset + (SafeCoord(coord.xy + int2(-1, 0))) / float(GridResolution);
    const float pressureLeft = PressureTexIn.Sample(g_BindlessSamplers[samplerIndex], uvLeft);
    
    const float2 uvRight = centerOfCellOffset + (SafeCoord(coord.xy + int2(1, 0))) / float(GridResolution);
    const float pressureRight = PressureTexIn.Sample(g_BindlessSamplers[samplerIndex], uvRight);
    
    const float2 uvUp = centerOfCellOffset + (SafeCoord(coord.xy + int2(0, -1))) / float(GridResolution);
    const float pressureUp = PressureTexIn.Sample(g_BindlessSamplers[samplerIndex], uvUp);
    
    const float2 uvDown = centerOfCellOffset + (SafeCoord(coord.xy + int2(0, 1))) / float(GridResolution);
    const float pressureDown = PressureTexIn.Sample(g_BindlessSamplers[samplerIndex], uvDown);
    
    float pressureNeighbours = pressureLeft + pressureRight + pressureUp + pressureDown;
    float newPressure = ( pressureNeighbours - div) * 0.25f;
    PressureTexOut[DTid.xy] = newPressure;
}


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain_FixEdges(uint3 DTid : SV_DispatchThreadID)
{
    int2 coord = int2(DTid.xy);
    PressureTexOut[DTid.xy] = PressureTexIn.Load(int3(clamp(coord.x, 1, GridResolution - 2), clamp(coord.y, 1, GridResolution - 2), 0) );
}

