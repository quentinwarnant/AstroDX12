#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float> DivergenceTex : register(t0);
Texture2D<float> PressureTexIn : register(t1);
RWTexture2D<float> PressureTexOut : register(u0);

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
    float h = 1.0f / GridResolution; // grid spacing
    float h2 = h * h; // grid spacing , squared
    
    float div = DivergenceTex[DTid.xy];
    float pressureLeft = PressureTexIn[SafeCoord(DTid.xy + uint2(-1, 0))];
    float pressureRight = PressureTexIn[SafeCoord(DTid.xy + uint2(1, 0))];
    float pressureUp = PressureTexIn[SafeCoord(DTid.xy + uint2(0, 1))];
    float pressureDown = PressureTexIn[SafeCoord(DTid.xy + uint2(0, -1))];
    
    float pressureNeighbours = pressureLeft + pressureRight + pressureUp + pressureDown;
    float newPressure = (pressureNeighbours - (h2 * div)) * 0.25f; 
    PressureTexOut[DTid.xy] = newPressure;
}


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain_FixEdges(uint3 DTid : SV_DispatchThreadID)
{
    //TODO - only dispatch for edge cells
    if(DTid.x == 0 || DTid.x == GridResolution - 1 || DTid.y == 0 || DTid.y == GridResolution - 1)
    {
        PressureTexOut[DTid.xy] = 0.0f;
    }
    else
    {
        PressureTexOut[DTid.xy] = PressureTexIn[DTid.xy];
    }
}

