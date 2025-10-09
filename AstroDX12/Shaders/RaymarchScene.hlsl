
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

    const float2 RenderTargetPixelSize = float2(GBufferWidth, GBufferHeight);
    float2 UV = ((DTid.xy + 0.5) / RenderTargetPixelSize);
    UV.y = 1.f - UV.y; // flip vertically so origin is in bottom left corner.
    float2 ndc = (UV * 2.f) - 1.f; // Center origin and use -1 to 1 range
    const float AspectRatio = RenderTargetPixelSize.x / RenderTargetPixelSize.y;
    ndc.x *= AspectRatio;
    
    const float3 RayOrigin = gEyePosW;
    float3 rayDirInCamSpace = float3(ndc.x, ndc.y, 1.0f);
    
    float3 RayDir = normalize(
        mul(InvView, float4(rayDirInCamSpace, 0.f)).xyz
    );
    
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

        distanceTravelled += DistToClosestObject;
        RayPos += (RayDir * DistToClosestObject);
        
        if (DistToClosestObject <= minDistForCollision)
        {
            Hit = true;
            break;
        }
        
        if (Step == RaymarchStepCount - 1 || distanceTravelled >= WorldSize)
        {
            distanceTravelled = WorldSize; // No hits
            break;
        }
    }
        
    // Normalize distance to [0, 1] range
    distanceTravelled = 1.f - saturate(distanceTravelled / WorldSize);
    RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[OutputTextureDepthResourceIndex];
    outputTexture[DTid.xy] = float4(distanceTravelled,  0.f, 0.f, 0.0f);
}