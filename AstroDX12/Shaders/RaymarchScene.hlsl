
cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 InvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float gPerObjectPad1; // Unused - padding
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

cbuffer BindlessRenderResources : register(b1)
{
    int SDFSceneObjectsResourceIndex;
    int OutputTextureDepthResourceIndex;
    int SDFObjectCount;
    int GBufferWidth;
    int GBufferHeight;
}

//struct SDFSceneObject
//{
//    float4 Pos;    
//};

struct SDFSceneObject //ParticleData
{
    float3 Pos;
    float3 Vel;
    float Age;
    float Lifetime;
    float Size;
};

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    StructuredBuffer<SDFSceneObject> SDFSceneBuffer = ResourceDescriptorHeap[SDFSceneObjectsResourceIndex];
    const int RaymarchStepCount = 100;
    const float MaxDistancePerStep = 30.f; 
    const float WorldSize = gFarZ - gNearZ;

    float3 right = float3(1, 0, 0);
    float3 up = float3(0, 1, 0);

    const float2 RenderTargetPixelSize = float2(GBufferWidth, GBufferHeight);
    float2 UV = (DTid.xy / RenderTargetPixelSize) * 2.f - 1.f;
    const float AspectRatio = RenderTargetPixelSize.x / RenderTargetPixelSize.y;
    UV.x *= AspectRatio;

    const float3 RayOrigin = gEyePosW;
    const float3 RayDir = normalize( /*Right*/gView[0].xyz * UV.x + /*Up*/gView[1].xyz * UV.y + /*Forward*/gView[2].xyz * 1.f);

//    const float3 RayOrigin = float3(0, 0, -10);
//    const float3 RayDir = normalize(float3(UV.x, UV.y, 1.f));
    
    const float minDistForCollision = 0.0001f;
    float distanceTravelled = 0.f;
    float3 RayPos = RayOrigin;

    bool Hit = false;
    for (int Step = 0; Step < RaymarchStepCount; ++Step)
    {
        float DistToClosestObject = MaxDistancePerStep; // minimum distance to travel at each step

        // for loop over all the objects near the sampling point
        for (int objIdx = 0; objIdx < SDFObjectCount; ++objIdx)
        {
            SDFSceneObject SDFObject = SDFSceneBuffer[objIdx];    

            const float3 objToSamplePos = (SDFObject.Pos.xyz - RayPos);
            float distToCenter = length(objToSamplePos);

            // Keep minimum distance between all evaluated objects to travel the next jump
            DistToClosestObject = min(DistToClosestObject, distToCenter - SDFObject.Size);
        }

        if (DistToClosestObject <= minDistForCollision)
        {
            Hit = true;
            break;
        }

        if (Hit)
        {
            break;
        }

        distanceTravelled += DistToClosestObject;
        RayPos = RayOrigin + (RayDir * distanceTravelled);
        
        if (Step == RaymarchStepCount - 1 || distanceTravelled >= WorldSize)
        {
            distanceTravelled = 0.f; // No hits
            break;
        }
    }
        

    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[OutputTextureDepthResourceIndex];
    outputTexture[DTid.xy] = float4(distanceTravelled, 0.f, 0.f, 0.0f);
}