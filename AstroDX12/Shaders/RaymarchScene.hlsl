#include "Shaders/ParticlesCommon.hlsli"

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
    int OutputTextureColorResourceIndex;
    int SDFObjectCount;
    int GBufferWidth;
    int GBufferHeight;
}

struct SminResult
{
    float dist;
    float blend; // 0 = fully a, 1 = fully b
};

SminResult smin(float a, float b, float k)
{
    float h = saturate(0.5 + 0.5 * (b - a) / k);
    float d = lerp(b, a, h) - k * h * (1.0 - h);
    SminResult result;
    result.dist = d;
    result.blend = h;
    return result;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    StructuredBuffer<ParticleData> SDFSceneBuffer = ResourceDescriptorHeap[SDFSceneObjectsResourceIndex];
    const int RaymarchStepCount = 100;
    const float MaxDistancePerStep = 30.f; 
    const float WorldSize = gFarZ - gNearZ;

    const float2 RenderTargetPixelSize = float2(GBufferWidth, GBufferHeight);
    float2 UV = ((DTid.xy + 0.5) / RenderTargetPixelSize);
    float2 ndc = (UV * 2.f) - 1.f;
    const float3 RayOrigin = gEyePosW;
    float4 rayEndCamSpace = mul(float4(ndc, 1.0f, 1.0f), gInvProj);
    float3 rayDirInCamSpace = rayEndCamSpace.xyz / rayEndCamSpace.w;
    
    float3 RayDir = normalize(
        mul(float4(rayDirInCamSpace, 0.f), InvView).xyz
    );
    
    const float minDistForCollision = 0.0001f;
    float distanceTravelled = 0.f;
    float3 RayPos = RayOrigin;

    bool Hit = false;
    float3 HitNormal = float3(0.f, 0.f, 0.f);
    for (int Step = 0; Step < RaymarchStepCount; ++Step)
    {
        float DistToClosestObject = MaxDistancePerStep;
        float3 blendedNormal = float3(0.f, 0.f, 0.f);

        for (int objIdx = 0; objIdx < SDFObjectCount; ++objIdx)
        {
            ParticleData SDFObject = SDFSceneBuffer[objIdx];
            
            const float3 objToSamplePos = (SDFObject.Pos.xyz - RayPos);
            float distToCenter = length(objToSamplePos);
            float objDist = distToCenter - SDFObject.Size;
            float3 objNormal = -objToSamplePos / max(distToCenter, 0.0001f);

            // Smoothly blend distances and normals of multiple objects to create a combined SDF field
            static const float blendFactor = 15.f;
            SminResult result = smin(DistToClosestObject, objDist, blendFactor);
            blendedNormal = lerp(objNormal, blendedNormal, result.blend);
            DistToClosestObject = result.dist;
        }

        distanceTravelled += DistToClosestObject;
        RayPos += (RayDir * DistToClosestObject);
        
        if (DistToClosestObject <= minDistForCollision)
        {
            Hit = true;
            HitNormal = normalize(blendedNormal);
            break;
        }
        
        if (Step == RaymarchStepCount - 1 || distanceTravelled >= WorldSize)
        {
            distanceTravelled = WorldSize; // No hits
            break;
        }
    }
        
    // Normalize distance to [0, 1] range
    float normalizedDepth = 1.f - saturate(distanceTravelled / WorldSize);

    RWTexture2D<float> depthTexture = ResourceDescriptorHeap[OutputTextureDepthResourceIndex];
    depthTexture[DTid.xy] = normalizedDepth;

    RWTexture2D<float4> colorTexture = ResourceDescriptorHeap[OutputTextureColorResourceIndex];
    float4 shading = float4(0.f, 0.f, 0.f, 0.f);
    if( Hit )
    {
        static const float3 fakeLightDir = normalize(float3(0.5f, 1.0f, -0.5f));
        static const float3 fakeLightColor = normalize(float3(1.5f, 1.0f, 0.0f));
        const float nDotL = saturate(dot(HitNormal, fakeLightDir));
        const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
        shading = float4(ambientColor + (nDotL * fakeLightColor), 1.f);
    }
    colorTexture[DTid.xy] = shading;
}