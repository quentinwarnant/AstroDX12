#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

Texture2D<float2> BufferIn : register(t0);
RWTexture2D<float2> BufferOut : register(u0);

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    BufferOut[DTid.xy] = BufferIn[DTid.xy]+ 0.01f;
}