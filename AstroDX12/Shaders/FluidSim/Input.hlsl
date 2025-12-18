#define THREAD_GROUP_SIZE_X 32
#define THREAD_GROUP_SIZE_Y 32

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

Texture2D<float2> VelocityGridInput : register(t0);
Texture2D<float4> DensityGridInput : register(t1);
RWTexture2D<float2> VelocityGridOutput : register(u0);
RWTexture2D<float4> DensityGridOutput : register(u1);

cbuffer BindlessRenderResources : register(b1)
{
    int GridResolution;
    float time;
    int inputPosX;
    int inputPosY;
    int inputPrevPosX;
    int inputPrevPosY;
}

float2 GetInputInQuadUV(float2 inputScreenUV)
{
    float2 centeredInputScreenUV = (inputScreenUV - 0.5f) * 2.f; // -1. to 1.
    
    // TODO: get vertex positions from a structured buffer
    //StructuredBuffer<VertexData> vertexData = ResourceDescriptorHeap[modelVertexDataBufferIdx];
    
    // get Quad extrema in screen UV space
    float3 QuadBLWS = float3(-10, -10, 0);
    float3 QuadTRWS = float3(10, 10, 0);
    // Put in clipspace
    float4 QualBLProjSpace = mul(float4(QuadBLWS, 1.0f), gViewProj);
    float4 QualTRProjSpace = mul(float4(QuadTRWS, 1.0f), gViewProj);
    // Put in NDC space
    float3 QuadBLNDC = QualBLProjSpace.xyz / QualBLProjSpace.w;
    float3 QuadTRNDC = QualTRProjSpace.xyz / QualTRProjSpace.w;
    // Put in quad-screen UV space
    // Screenspace
    float2 QuadScreenUV_BL = (QuadBLNDC.xy * 0.5f) + 0.5f;
    float2 QuadScreenUV_TR = (QuadTRNDC.xy * 0.5f) + 0.5f;
    float2 inputPosUV = (inputScreenUV - QuadScreenUV_BL) / (QuadScreenUV_TR - QuadScreenUV_BL);
    
    return inputPosUV;
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    const float2 invViewPortSize = ((float2) 1.f) / gRenderTargetSize;
    const uint2 pixelCoord = uint2(inputPosX, gRenderTargetSize.y- inputPosY);
    const float2 inputScreenUV = pixelCoord * invViewPortSize;
    
    const float2 inputPosUV = GetInputInQuadUV(inputScreenUV);
    
    float2 uv = (float2(DTid.xy) + 0.5f) / float2(GridResolution, GridResolution);
    uv -= inputPosUV;
    
    const bool inBounds = any(inputPosUV >= 0.f) || any(inputPosUV <= 1.f);
    const float deltaTime = 1.f / 60.f; // todo make this an input
    
    const float InputVelocityScale = 300000.0f;
    const float2 inputCursorVelocity = float2(inputPosX - inputPrevPosX, inputPosY - inputPrevPosY) * invViewPortSize * InputVelocityScale;
    // Clamp velocity
    const float velocityMagSqr = dot(inputCursorVelocity, inputCursorVelocity);
    float2 addedVelocityDir = float2(0.f, 0.f);
    float velocityMagnitude = 0.f;
    if (velocityMagSqr > 0.001f)
    {
        addedVelocityDir = normalize(inputCursorVelocity); 
        velocityMagnitude = sqrt(velocityMagSqr);
        
        const float MaxVelocityMagnitude = 30000.f;
        if (velocityMagnitude > MaxVelocityMagnitude)
        {
            velocityMagnitude = MaxVelocityMagnitude;
        }
    }
    const float addedVelocityStrength = velocityMagnitude * deltaTime * (inBounds ? 1.f : 0.f);
    
    const float2 currentVelocity = VelocityGridInput[DTid.xy];
    const float2 inputVelocity = addedVelocityDir * addedVelocityStrength;
    const float maskRadius = 0.02f;
    const float mask = step(length(uv), maskRadius);
    VelocityGridOutput[DTid.xy] = lerp(currentVelocity, currentVelocity + (inputVelocity), mask);
    
    const float decayRate = 0.999f;
    const float4 density = DensityGridInput[DTid.xy] * decayRate;
    DensityGridOutput[DTid.xy] = min(1.f, lerp(density, 
    (float4((0.5 * sin(time)) + 1.f, 0.5f * (cos(time)) + 1.f, time % 1.f, 0.f)) * 1.0f
    , mask));

}